import re
def convert_suffix(match):
    num = match.group(1)
    suffix = match.group(2)
    suffix_map = {
        'a': 'E-18',
        'f': 'E-15',
        'u': 'E-6',
        'n': 'E-9',
        'p': 'E-12',
        'm': 'E-3',
        'e': 'E'
    }
    return num + suffix_map.get(suffix.lower(), '')

# if you want to modify .xml files under other processes, you may need to modify the path below.
xml_file = "~/vtr-verilog-to-routing/vtr_flow/tech/PTM_22nm/22nm.xml"

# read .xml file
with open(xml_file, 'r') as file:
    lines = file.readlines()

# Replace the units with the corresponding exponential level.
for i in range(1,len(lines)):
    lines[i] = re.sub(r'(\d+(?:\.\d*)?)([a-zA-Z])(?![a-zA-Z])', convert_suffix, lines[i])


# write the content to the .xml file
with open(xml_file, 'w') as file:
    file.writelines(lines)