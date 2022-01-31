#ifndef SHOWSETUP_H
#define SHOWSETUP_H

struct ClusteredNetlistStats {
  private:
    void writeHuman(std::ostream& output) const;
    void writeJSON(std::ostream& output) const;
    void writeXML(std::ostream& output) const;

  public:
    ClusteredNetlistStats();

    enum OutputFormat {
        HumanReadable,
        JSON,
        XML
    };

    int num_nets;
    int num_blocks;
    int L_num_p_inputs;
    int L_num_p_outputs;
    std::vector<int> num_blocks_type;
    std::vector<t_logical_block_type> logical_block_types;

    void write(OutputFormat fmt, std::ostream& output) const;
};

void ShowSetup(const t_vpr_setup& vpr_setup);
void writeClusteredNetlistStats(std::string block_usage_filename);

#endif
