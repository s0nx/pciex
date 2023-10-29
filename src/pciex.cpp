#include "pciex.h"

namespace pci {

PciDevice::PciDevice(uint16_t dom, uint8_t bus, uint8_t dev, uint8_t func,
                     cfg_space_type type, std::unique_ptr<uint8_t[]> cfg) :
    dom_(dom),
    bus_(bus),
    dev_(dev),
    func_(func),
    cfg_type_(type),
    cfg_space_(std::move(cfg)) {}

uint16_t PciDevice::get_vendor_id() const noexcept {
    auto cfg_buf = cfg_space_.get();
    auto vid = reinterpret_cast<uint16_t *>(cfg_buf + e_type(Type0Cfg::vid));
    return *vid;
}

uint16_t PciDevice::get_dev_id() const noexcept {
    auto cfg_buf = cfg_space_.get();
    auto dev_id = reinterpret_cast<uint16_t *>(cfg_buf + e_type(Type0Cfg::dev_id));
    return *dev_id;
}

uint32_t PciDevice::get_class_code() const noexcept {
    auto cfg_buf = cfg_space_.get();
    auto dword2 = reinterpret_cast<uint32_t *>(cfg_buf + e_type(Type0Cfg::revision));
    auto class_code = *dword2;
    return class_code >> 8;
}

void PciDevice::print_data() const noexcept {
    auto vid    = get_vendor_id();
    auto dev_id = get_dev_id();
    logger.info("[{:04}:{:02}:{:02x}.{}] -> cfg_size {:4} vendor {:2x} | dev {:2x}",
               dom_, bus_, dev_, func_,
               cfg_type_ == cfg_space_type::LEGACY ? pci_cfg_legacy_len : pci_cfg_ecs_len,
               vid, dev_id);
}

} // namespace pci
