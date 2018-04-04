#ifndef READ_ARCH_H
#define READ_ARCH_H
#include "pass.h"

class ReadArchPass : public DevicePass {
    public:

        ReadArchPass(const std::string arch_file_path)
            : arch_file_path_(arch_file_path) {}
        
        
        bool run_on_device(DeviceContext& device_ctx) override {
            XmlReadArch(arch_file_path_.c_str(), 
                        true,
                        &device_ctx.arch,
                        &device_ctx.block_types,
                        &device_ctx.num_block_types);
            return true;
        }

    private:
        std::string arch_file_path_;
};

#endif
