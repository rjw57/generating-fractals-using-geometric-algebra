PDFLATEX := pdflatex
TEXINPUTS := ../../elsevier-author-resources/:paper-figures/:.::

# Top-level target
all: figures fractals.pdf

figures:

clean:
	rm -f *.aux
	rm -f *.toc
	rm -f *.pdf
	rm -f *.log
	rm -f *.blg
	rm -f *.bbl

%.pdf: %.tex
	TEXINPUTS=$(TEXINPUTS) BSTINPUTS=$(TEXINPUTS) $(PDFLATEX) $<
	TEXINPUTS=$(TEXINPUTS) BSTINPUTS=$(TEXINPUTS) bibtex $(<:.tex=)
	TEXINPUTS=$(TEXINPUTS) BSTINPUTS=$(TEXINPUTS) $(PDFLATEX) $<
	TEXINPUTS=$(TEXINPUTS) BSTINPUTS=$(TEXINPUTS) $(PDFLATEX) $<

.PHONY: all figures clean
