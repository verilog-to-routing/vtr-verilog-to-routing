 #!/bin/bash

./generate_cmos_tech_data.pl ../tech/PTM_22nm/22nm.pm 22e-9 0.8 85 > ../tech/PTM_22nm/22nm.xml &
./generate_cmos_tech_data.pl ../tech/PTM_45nm/45nm.pm 45e-9 0.9 85 > ../tech/PTM_45nm/45nm.xml &
./generate_cmos_tech_data.pl ../tech/PTM_130nm/130nm.pm 130e-9 1.3 85 > ../tech/PTM_130nm/130nm.xml &
