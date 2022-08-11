TEX := pdflatex
#Tool options
TEX_OPTS := -file-line-error -halt-on-error -interaction=errorstopmode -shell-escape #-interaction=batchmode #-interaction=errorstopmode
# -pdf tells latexmk to generate PDF directly (instead of DVI).
# -pdflatex="" tells latexmk to call a specific backend with specific options.
# -use-make tells latexmk to call make for generating missing files.
LATEXMK_OPTS := -pdf -pdflatex="$(TEX) $(TEX_OPTS)" -recorder -use-make

FIG_TARGETS := vtr_flow_fig.pdf vtr_flow_fig.svg

.PHONY: clean distclean

all: $(FIG_TARGETS) 

clean:
	latexmk $(LATEXMK_OPTS) -C
	rm -f $(FIG_TARGETS)

%.pdf: %.tex
	latexmk $(LATEXMK_OPTS) $^

%.svg: %.pdf
	pdf2svg $^ $@
