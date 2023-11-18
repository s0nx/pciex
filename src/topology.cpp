#include "pciex.h"

namespace pci {

static auto pci_dev_get_data(fs::path &pci_dev_entry)
{
    auto config = fs::directory_entry(pci_dev_entry.concat("/config"));
    auto cfg_size = config.file_size();

    auto cfg_fd = std::fopen(config.path().c_str(), "r");
    if (!cfg_fd)
        throw CfgEx(fmt::format("Failed to open {}", config.path().string()));

    std::unique_ptr<uint8_t[]> ptr { new (std::nothrow) uint8_t [cfg_size] };
    if (!ptr)
    {
        std::fclose(cfg_fd);
        throw CfgEx(fmt::format("Failed to allocate cfg buffer for {}", config.path().string()));
    }

    const std::size_t read = std::fread(ptr.get(), cfg_size, 1, cfg_fd);
    if (read != 1)
    {
        std::fclose(cfg_fd);
        throw CfgEx(fmt::format("Failed to read cfg buffer for {}", config.path().string()));
    }

    std::fclose(cfg_fd);

    auto h_type = reinterpret_cast<uint8_t *>(ptr.get() + e_to_type(Type0Cfg::header_type));
    pci_dev_type dev_type = *h_type & 0x1 ? pci_dev_type::TYPE1 : pci_dev_type::TYPE0;

    return std::make_tuple(std::move(ptr),
                           cfg_size == e_to_type(cfg_space_type::ECS) ?
                           cfg_space_type::ECS : cfg_space_type::LEGACY, dev_type);
}

void PCITopologyCtx::populate()
{
    const auto bdf_regex = std::regex("([a-f0-9]{4}):([a-f0-9]{2}):([a-f0-9]{2}).(.)");
    auto mres = std::smatch{};

    const fs::path sys_pci_devs_path {pci_dev_path};
    logger.info("Scanning {}...", sys_pci_devs_path.c_str());

    for (const auto &pci_dev_dir_e : fs::directory_iterator {sys_pci_devs_path}) {
        auto entry = fs::absolute(pci_dev_dir_e);

        std::string p = entry.string();
        bool res = std::regex_search(p, mres, bdf_regex);
        if (!res || mres.size() < 5) {
            throw CfgEx(fmt::format("Failed to parse BDF for {}\n", p));
        } else {
            auto dom  = static_cast<uint16_t>(std::stoull(mres[1].str(), nullptr, 16));
            auto bus  = static_cast<uint8_t>(std::stoull(mres[2].str(), nullptr, 16));
            auto dev  = static_cast<uint8_t>(std::stoull(mres[3].str(), nullptr, 16));
            auto func = static_cast<uint8_t>(std::stoull(mres[4].str(), nullptr, 16));

            logger.info("Got -> [{:04}:{:02}:{:02x}.{}]", dom, bus, dev, func);

            uint64_t d_bdf = func | (dev << 8) | (bus << 16) | (dom << 24);
            auto [data, cfg_type, dev_type] = pci::pci_dev_get_data(entry);
            auto dev_ptr = dev_creator_.create(d_bdf, cfg_type, dev_type, std::move(data));
            devs_.push_back(std::move(dev_ptr));
        }
    }
}

void PCITopologyCtx::dump_data() const noexcept
{
    for (const auto &el : devs_)
        el->print_data();
}

} // namespace pci
