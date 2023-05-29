import numpy as np
import warnings
from joblib import load
warnings.filterwarnings("ignore")
clf_dt = load('dt_saved.joblib')
with open('optimal_ratio.txt', 'w') as file:
	ratio_dict = {4: 0.75, 1: 0.2, 8: 1.0, 3: 0.7, 0: 0.0, 2: 0.25, 6: 0.9, 7: 0.95, 5: 0.85}
	x = [0,0,0,0]
	with open ('../vtr_flow/scripts/temp/netstats.txt', 'r') as s:
	    for i in s:
        	if "Memory" in i:
        	    j = i.split(": ")
        	    x[0]=(int(j[-1]))
        	if "MULTIPLY" in i:
        	    j = i.split(": ")
        	    x[1]=(int(j[-1]))
        	if "lut" in i:
        	    j = i.split(": ")
        	    x[2]=(int(j[-1]))
        	if "Longest" in i:
        	    j = i.split(": ")
        	    x[3]=(int(j[-1]))
	X_test = np.array([x])
	Y_pred_gnb = clf_dt.predict(X_test)
	print()
	print("Optimal Ratio = ", ratio_dict[Y_pred_gnb[0]])
	print()
	file.write(str(ratio_dict[Y_pred_gnb[0]]))
