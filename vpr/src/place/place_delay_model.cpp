#include <queue>
#include "place_delay_model.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "rr_graph2.h"

#include "timing_place_lookup.h"

#include "vtr_log.h"
#include "vtr_math.h"

#include "capnp/serialize.h"
#include "lookahead.capnp.h"
#include "ndmatrix_serdes.h"
#include "mmap_file.h"
#include "serdes_utils.h"

/*
 * DeltaDelayModel
 */

float DeltaDelayModel::delay(int from_x, int from_y, int /*from_pin*/, int to_x, int to_y, int /*to_pin*/) const {
    int delta_x = std::abs(from_x - to_x);
    int delta_y = std::abs(from_y - to_y);

    return delays_[delta_x][delta_y];
}

void DeltaDelayModel::dump_echo(std::string filepath) const {
    FILE* f = vtr::fopen(filepath.c_str(), "w");
    fprintf(f, "         ");
    for (size_t dx = 0; dx < delays_.dim_size(0); ++dx) {
        fprintf(f, " %9zu", dx);
    }
    fprintf(f, "\n");
    for (size_t dy = 0; dy < delays_.dim_size(1); ++dy) {
        fprintf(f, "%9zu", dy);
        for (size_t dx = 0; dx < delays_.dim_size(0); ++dx) {
            fprintf(f, " %9.2e", delays_[dx][dy]);
        }
        fprintf(f, "\n");
    }
    vtr::fclose(f);
}

/*
 * OverrideDelayModel
 */
float OverrideDelayModel::delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const {
    //First check to if there is an override delay value
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    t_type_ptr from_type_ptr = grid[from_x][from_y].type;
    t_type_ptr to_type_ptr = grid[to_x][to_y].type;

    t_override override_key;
    override_key.from_type = from_type_ptr->index;
    override_key.from_class = from_type_ptr->pin_class[from_pin];
    override_key.to_type = to_type_ptr->index;
    override_key.to_class = to_type_ptr->pin_class[to_pin];

    //Delay overrides may be different for +/- delta so do not use
    //an absolute delta for the look-up
    override_key.delta_x = to_x - from_x;
    override_key.delta_y = to_y - from_y;

    float delay_val = std::numeric_limits<float>::quiet_NaN();
    auto override_iter = delay_overrides_.find(override_key);
    if (override_iter != delay_overrides_.end()) {
        //Found an override
        delay_val = override_iter->second;
    } else {
        //Fall back to the base delay model if no override was found
        delay_val = base_delay_model_->delay(from_x, from_y, from_pin, to_x, to_y, to_pin);
    }

    return delay_val;
}

void OverrideDelayModel::set_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y, float delay_val) {
    t_override override_key;
    override_key.from_type = from_type;
    override_key.from_class = from_class;
    override_key.to_type = to_type;
    override_key.to_class = to_class;
    override_key.delta_x = delta_x;
    override_key.delta_y = delta_y;

    auto res = delay_overrides_.insert(std::make_pair(override_key, delay_val));
    if (!res.second) {                 //Key already exists
        res.first->second = delay_val; //Overwrite existing delay
    }
}

void OverrideDelayModel::dump_echo(std::string filepath) const {
    base_delay_model_->dump_echo(filepath);

    FILE* f = vtr::fopen(filepath.c_str(), "a");

    fprintf(f, "\n");
    fprintf(f, "# Delay Overrides\n");
    auto& device_ctx = g_vpr_ctx.device();
    for (auto kv : delay_overrides_) {
        auto override_key = kv.first;
        float delay_val = kv.second;
        fprintf(f, "from_type: %s to_type: %s from_pin_class: %d to_pin_class: %d delta_x: %d delta_y: %d -> delay: %g\n",
                device_ctx.block_types[override_key.from_type].name,
                device_ctx.block_types[override_key.to_type].name,
                override_key.from_class,
                override_key.to_class,
                override_key.delta_x,
                override_key.delta_y,
                delay_val);
    }

    vtr::fclose(f);
}

static void ToFloat(float *out, const Vpr::FloatEntry::Reader &in) {
    *out = in.getValue();
}

static void FromFloat(Vpr::FloatEntry::Builder *out, const float &in) {
    out->setValue(in);
}

void DeltaDelayModel::read(const std::string& file) {
    MmapFile f(file);
    ::capnp::FlatArrayMessageReader reader(f.getData());

    auto model = reader.getRoot<Vpr::DeltaDelayModel>();
    ToNdMatrix<2, Vpr::FloatEntry, float>(&delays_, model.getDelays(), ToFloat);
}

void DeltaDelayModel::write(const std::string& file) const {
    ::capnp::MallocMessageBuilder builder;
    auto model = builder.initRoot<Vpr::DeltaDelayModel>();

    auto delays = model.getDelays();
    FromNdMatrix<2, Vpr::FloatEntry, float>(&delays, delays_, FromFloat);

    writeMessageToFile(file, &builder);
}

void OverrideDelayModel::read(const std::string& file) {
    MmapFile f(file);
    ::capnp::FlatArrayMessageReader reader(f.getData());

    vtr::Matrix<float> delays;
    auto model = reader.getRoot<Vpr::OverrideDelayModel>();
    ToNdMatrix<2, Vpr::FloatEntry, float>(&delays, model.getDelays(), ToFloat);

    base_delay_model_ = std::make_unique<DeltaDelayModel>(delays);

    auto overrides = model.getDelayOverrides();
    std::vector<std::pair<t_override, float>> overrides_arr(overrides.size());
    for(size_t i = 0; i < overrides.size(); ++i) {
        const auto &elem = overrides[i];
        overrides_arr[i].first.from_type = elem.getFromType();
        overrides_arr[i].first.to_type = elem.getToType();
        overrides_arr[i].first.from_class = elem.getFromClass();
        overrides_arr[i].first.to_class = elem.getToClass();
        overrides_arr[i].first.delta_x = elem.getDeltaX();
        overrides_arr[i].first.delta_y = elem.getDeltaY();

        overrides_arr[i].second = elem.getDelay();
    }

    delay_overrides_ = vtr::make_flat_map2(std::move(overrides_arr));
}

void OverrideDelayModel::write(const std::string& file) const {
    ::capnp::MallocMessageBuilder builder;
    auto model = builder.initRoot<Vpr::OverrideDelayModel>();

    auto delays = model.getDelays();
    FromNdMatrix<2, Vpr::FloatEntry, float>(&delays, base_delay_model_->delays(), FromFloat);

    auto overrides = model.initDelayOverrides(delay_overrides_.size());
    auto dst_iter = overrides.begin();
    for(const auto & src : delay_overrides_) {
        auto elem = *dst_iter++;
        elem.setFromType(src.first.from_type);
        elem.setToType(src.first.to_type);
        elem.setFromClass(src.first.from_class);
        elem.setToClass(src.first.to_class);
        elem.setDeltaX(src.first.delta_x);
        elem.setDeltaY(src.first.delta_y);

        elem.setDelay(src.second);
    }

    writeMessageToFile(file, &builder);
}
