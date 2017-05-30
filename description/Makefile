SHELL = /bin/bash

all: bibl main
	pdflatex main.tex

main: main.tex 
	pdflatex main.tex

bibl: $(wildcard bib/*.bib)
	pdflatex main.tex
	BIBINPUTS=./bib bibtex main

clean:
	rm -f *.aux *.log *.out *.bbl *.blg *.brf

realclean: clean
	rm -f main.pdf
