import csv
import re
import os
from lxml import etree

def generate_hecate_interposer_archs(csv_file, template_path, output_dir="hecate_25D"):
    """
    Generates customized VPR architecture XML files by injecting varying scatter-gather
    and inter-die connectivity configurations into a base template XML.
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    if not os.path.exists(template_path):
        print(f"Error: Template {template_path} not found.")
        return

    # Using lxml parser to retain existing XML comments during tree transformation
    parser = etree.XMLParser(remove_comments=False)
    
    # Define the range of fan-in and fan-out sizes for interposer routing exploration
    gather_conn_sizes = ["3", "6", "12", "24", "48"]
    scatter_conn_sizes = ["4", "8", "16", "32", "64"]

    with open(csv_file, mode='r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        
        for row in reader:
            arch_id = row['arch_id']
            mux_name = row['mux_name']
            wire_name = row['wire_name'] 
            csv_num = row['num']
            
            # Extract the interposer segment length
            match = re.search(r'\d+', wire_name)
            L = int(match.group()) if match else 1

            for gather_n_val, scatter_n_val in zip(gather_conn_sizes, scatter_conn_sizes):
                # Initialize a fresh XML tree from the template for each configuration
                tree = etree.parse(template_path, parser)
                root = tree.getroot()

                # Configure physical segment properties (e.g. length and driver mux name) for the inter-die wire
                segment = root.xpath(".//segmentlist/segment[@name='int_wire']")
                if segment:
                    seg = segment[0]
                    seg.set("length", str(L))
                    
                    mux_tag = seg.find("mux")
                    if mux_tag is not None:
                        mux_tag.set("name", mux_name)

                    # Pattern logic: sb = L+1, cb = L
                    sb_tag = seg.xpath("./sb[@type='pattern']")
                    if sb_tag:
                        sb_tag[0].text = " ".join(["0"] * (L + 1))

                    cb_tag = seg.xpath("./cb[@type='pattern']")
                    if cb_tag:
                        cb_tag[0].text = " ".join(["0"] * L)

                # Update scatter-gather pattern definitions which model the multi-die connections.
                # This specifies the number of upward and downward incoming/outgoing connections.
                patterns = {
                    "downward": root.xpath(".//scatter_gather_list/sg_pattern[@name='interposer_sg_downward']"),
                    "upward": root.xpath(".//scatter_gather_list/sg_pattern[@name='interposer_sg_upward']")
                }

                for direction, sg_list in patterns.items():
                    if not sg_list:
                        continue
                    sg = sg_list[0]

                    # Update num_conns for gather and scatter (3, 6, 12, 24, or 38)
                    for tag_name in ["gather", "scatter"]:
                        wireconn = sg.xpath(f"./{tag_name}/wireconn")
                        if wireconn:
                            if tag_name == "gather":
                                wireconn[0].set("num_conns", gather_n_val)
                            else:
                                wireconn[0].set("num_conns", scatter_n_val)

                    # Update the sg_link Y offsets depending on the interposer routing direction
                    sg_link = sg.xpath("./sg_link_list/sg_link")
                    if sg_link:
                        offset_val = -L if direction == "downward" else L
                        sg_link[0].set("y_offset", str(offset_val))

                # Adjust starting and ending coordinates for top-level interposer cut wires.
                # These specify where interposer connections can legally form on the top level grid.
                up_wire = root.xpath(".//layout/auto_layout/interposer_cut/interdie_wire[@sg_name='interposer_sg_upward']")
                if up_wire:
                    up_wire[0].set("offset_start", str(-(L - 1)))
                    up_wire[0].set("offset_end", "-1")
                    up_wire[0].set("num", csv_num)

                down_wire = root.xpath(".//layout/auto_layout/interposer_cut/interdie_wire[@sg_name='interposer_sg_downward']")
                if down_wire:
                    down_wire[0].set("offset_start", "1")
                    down_wire[0].set("offset_end", str(L - 1))
                    down_wire[0].set("num", csv_num)

                # Export the fully customized architecture description to a new XML file
                output_filename = f"hecate_25d_{arch_id}_fanin_{gather_n_val}.xml"
                output_path = os.path.join(output_dir, output_filename)
                
                tree.write(output_path, encoding="UTF-8", xml_declaration=True, pretty_print=True)
                print(f"Generated: {output_path}")

if __name__ == "__main__":
    generate_hecate_interposer_archs('int_connectivity.csv', 'hecate_25d_L17_int_10um_bump_fanin_12.xml')