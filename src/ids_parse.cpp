#include "ids_parse.h"

using namespace pci;

PciIdParser::PciIdParser()
{
    auto ids_db_path = fs::path(pci_ids_db_path);
    auto ids_db_entry = fs::directory_entry(ids_db_path);
    db_size_ = ids_db_entry.file_size();

    logger.info("PCI ids path: {} -> size: {}", ids_db_path.string(), db_size_);
    buf_ = std::make_unique<char[]>(db_size_);

    auto db_fd = std::fopen(ids_db_path.c_str(), "r");
    if (!db_fd)
        throw IdParseEx(fmt::format("Failed to open PCI ids db {}", ids_db_path.string()));

    auto res = std::fread(buf_.get(), db_size_, 1, db_fd);
    if (res != 1)
    {
        std::fclose(db_fd);
        throw IdParseEx(fmt::format("Failed to read PCI ids db: {}", ids_db_path.string()));
    }

    std::fclose(db_fd);

    db_str_ = std::string_view(buf_.get(), db_size_);
    ids_cache_.clear();
}

std::string_view PciIdParser::vendor_name_lookup(const uint16_t vid)
{
    /* search in cache first */
    auto cached_vid_desc = ids_cache_.find(vid);
    if (cached_vid_desc != ids_cache_.end())
    {
        logger.info("Found cached vendor desc for VID {:x}", vid);
        return cached_vid_desc->second.vendor_name_;
    }

    auto tgt_vid_str = fmt::format("\n{:04x}", vid);

    auto vid_pos = db_str_.find(tgt_vid_str);
    if (vid_pos == std::string_view::npos)
    {
        logger.info("Could not find vendor name for ID {:x}", vid);
        return std::string_view{};
    }

    /* '\n' (1 char) + VID len (4 chars) + two WS (2 chars) */
    constexpr off_t off = 1 + 4 + 2;
    auto vid_str_end_pos = db_str_.find('\n', vid_pos + off);
    if (vid_str_end_pos == std::string_view::npos)
    {
        logger.info("Could not parse vendor name for ID {:x}", vid);
        return std::string_view{};
    }

    auto vid_str = db_str_.substr(vid_pos + off, vid_str_end_pos - vid_pos - off);

    /* Cache found entry. @vendor_db_off_ holds the position in buffer
     * right after the vendor name string
     */
    auto cache_upd_res = ids_cache_.emplace(std::make_pair(vid,
                                   CachedDbVendorEntry(vid_str, vid_str_end_pos + 1)));
    if (!cache_upd_res.second)
    {
        logger.info("Could not cache parsed vendor name for ID {:x}", vid);
    }

    return vid_str;
}

std::string_view PciIdParser::device_name_lookup(const uint16_t vid,
                                                   const uint16_t dev_id)
{
    /* vendor name and db offset should have been cached */
    auto cached_vid_desc = ids_cache_.find(vid);
    if (cached_vid_desc == ids_cache_.end())
    {
        /* should always be present */
        logger.info("Cached vendor desc for VID {:x} has not been found", vid);
        return std::string_view{};
    }

    /* Try to obtain device name from cache */
    auto cached_dev_desc = cached_vid_desc->second.devs_.find(dev_id);
    if (cached_dev_desc != cached_vid_desc->second.devs_.end()) {
        return cached_dev_desc->second.device_name_;
    } else {
        /* nothing is cached, start searching from @vendor_db_off_ */
        auto tgt_dev_str = fmt::format("\t{:04x}", dev_id);
        auto dev_id_pos = db_str_.find(tgt_dev_str, cached_vid_desc->second.vendor_db_off_);
        if (dev_id_pos == std::string_view::npos) {
            logger.info("Could not parse device name for ID {:x}", dev_id);
            return std::string_view{};
        }

        /* '\t' + dev ID len + two WS */
        constexpr off_t off = 1 + 4 + 2;
        auto dev_name_start_pos = dev_id_pos + off;
        auto dev_name_end_pos = db_str_.find('\n', dev_name_start_pos);

        auto dev_name_str = db_str_.substr(dev_name_start_pos,
                                          dev_name_end_pos - dev_name_start_pos);

        /* Cache found dev name entry. @device_db_off_ holds the position in buffer
         * right after device name string. This might be useful when searching for
         * Subsystem ID/Subsystem Vendor ID pair later */
        auto res = cached_vid_desc->second.devs_.emplace(
            std::make_pair(dev_id, CachedDbDevEntry(dev_name_str, dev_name_end_pos + 1))
        );

        if (!res.second)
            logger.info("Could not cache parsed device name for ID {:x}", dev_id);

        return dev_name_str;
    }
}

std::string_view PciIdParser::subsys_name_lookup(const uint16_t vid, const uint16_t dev_id,
                                            const uint16_t subsys_vid, const uint16_t subsys_id)
{
    /* vendor name and db offset should have been cached */
    auto cached_vid_desc = ids_cache_.find(vid);
    if (cached_vid_desc == ids_cache_.end())
    {
        /* should be present */
        logger.info("Cached vendor desc for VID {} has not been found", vid);
        return std::string_view{};
    }

    auto cached_dev_desc = cached_vid_desc->second.devs_.find(dev_id);
    if (cached_dev_desc == cached_vid_desc->second.devs_.end()) {
        logger.info("Cached device desc for ID {} has not been found", dev_id);
        return std::string_view{};
    }

    auto subsys_str = fmt::format("\t\t{:04x} {:04x}", subsys_vid, subsys_id);
    auto subsys_str_pos = db_str_.find(subsys_str, cached_dev_desc->second.device_db_off_);
    if (subsys_str_pos == std::string_view::npos)
    {
        logger.info("Could not find subsystem name for subsys VID/subsys ID {} : {}",
                    subsys_vid, subsys_id);
        return std::string_view{};
    }

    // two '\t' + subsys VID len + WS + subsys ID + two WS
    constexpr off_t off = 2 + 4 + 1 + 4 + 2;
    auto subsys_name_spos = subsys_str_pos + off;
    auto subsys_name_epos = db_str_.find('\n', subsys_name_spos);

    auto subsys_name_str = db_str_.substr(subsys_name_spos, subsys_name_epos - subsys_name_spos);
    return subsys_name_str;
}

ClassCodeInfo PciIdParser::class_info_lookup(const uint32_t ccode)
{
    if (class_code_db_off_ == 0) {
        auto class_block_start = db_str_.rfind("C 00");
        if (class_block_start == std::string_view::npos) {
            logger.info("Failed to find class information block in PCI IDs db");
            return {{},{},{}};
        } else {
            logger.info("Found class information block at off {}", class_block_start);
            class_code_db_off_ = class_block_start;
        }
    }

    uint32_t cc[] {ccode};
    const auto cc_bytes = std::as_bytes(std::span{cc});
    const auto base_class_code = std::to_integer<uint8_t>(cc_bytes[2]);
    const auto sub_class_code  = std::to_integer<uint8_t>(cc_bytes[1]);
    const auto prog_iface      = std::to_integer<uint8_t>(cc_bytes[0]);

    logger.raw("CC: |base class {:02x}| subclass {:02x}| prog-if {:02x}|",
               base_class_code, sub_class_code, prog_iface);

    auto search_str = fmt::format("C {:02x}", base_class_code);
    auto search_pos = db_str_.find(search_str, class_code_db_off_);

    logger.raw("class pos: {}", search_pos);

    if (search_pos == std::string_view::npos) {
        logger.info("Failed to find base class code name for {}", base_class_code);
        return {{},{},{}};
    }

    // 'C' + WS + two digits + two WS
    off_t off = 1 + 1 + 2 + 2;
    auto name_spos = search_pos + off;
    auto name_epos = db_str_.find('\n', name_spos);

    logger.raw("class -> pos: {} epos: {}", search_pos, name_epos);
    auto class_name = db_str_.substr(name_spos, name_epos - name_spos);

    // find next class name entry which would act as a searching limit
    // during subclass search below
    auto search_limit_pos = db_str_.find("\nC ", name_epos);
    logger.raw("NEXT class pos: {}", search_limit_pos);
    if (search_limit_pos == std::string_view::npos) {
        // current class entry is probably the last one
        search_limit_pos = db_size_;
    }

    // try to find subclass now
    search_str = fmt::format("\n\t{:02x}", sub_class_code);
    search_pos = db_str_.find(search_str, name_epos);
    logger.raw("subclass pos: {}", search_pos);
    if (search_pos == std::string_view::npos || search_pos >= search_limit_pos) {
        logger.info("Failed to find sub class code name for {}", sub_class_code);
        return {class_name, {}, {}};
    }

    //  '\n' + \t' + two digits + two WS
    off = 1 + 1 + 2 + 2;
    name_spos = search_pos + off;
    name_epos = db_str_.find('\n', name_spos);
    logger.raw("subclass end pos: {}", name_epos);
    auto subclass_name = db_str_.substr(name_spos, name_epos - name_spos);

    if (name_epos + 1 == db_size_) {
        logger.info("Failed to find programming interface name for {}: EOF", prog_iface);
        return {class_name, subclass_name, {}};
    }

    // find the beginning of the next subclass entry
    auto cur_limit_pos = search_limit_pos;
    auto cur_off = name_epos;
    logger.raw("entering subclass loop at pos {} cur_limit {}", name_epos, cur_limit_pos);

    while (true) {
        search_limit_pos = db_str_.find("\n\t", cur_off);
        logger.raw("cur_limit_pos {}", search_limit_pos);
        if (search_limit_pos >= cur_limit_pos)
            break;

        if (search_limit_pos == std::string_view::npos) {
            logger.info("Failed to find subclass pattern");
            break;
        }

        if (db_str_[search_limit_pos + 2] == '\t') {
            search_limit_pos += 2;
        } else {
            logger.raw("found next subclass entry at {}", search_limit_pos + 2);
            search_limit_pos += 2;
            break;
        }
        cur_off = search_limit_pos;
    }

    logger.raw("next subclass pos: {}", search_limit_pos);

    // try to find programming interface now
    search_str = fmt::format("\t\t{:02x}", prog_iface);
    search_pos = db_str_.find(search_str, name_epos);
    logger.raw("prog iface pos: {}", search_pos);
    if (search_pos == std::string_view::npos ||
            search_pos >= search_limit_pos) {
        logger.info("Failed to find programming interface name for {}", prog_iface);
        return {class_name, subclass_name, {}};
    }

    // two '\t' + two digits + two WS
    off = 2 + 2 + 2;
    name_spos = search_pos + off;
    name_epos = db_str_.find('\n', name_spos);
    logger.raw("prog iface end pos: {}", name_epos);
    auto prog_iface_name = db_str_.substr(name_spos, name_epos - name_spos);

    return {class_name, subclass_name, prog_iface_name};
}


