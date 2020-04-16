SHELL = /bin/bash

all: main2019 references
	pdflatex main2019.tex

main2017: main2017.tex references
	pdflatex main2017.tex

main2018: main2018.tex references
	pdflatex main2018.tex

main2019: main2019.tex references
	pdflatex main2019.tex
	
references: main.bib
	pdflatex main2017.tex
	pdflatex main2018.tex
	pdflatex main2019.tex
	bibtex main2017
	bibtex main2018
	bibtex main2019

clean:
	rm -f *.aux *.bbl *.blg *.brf *.log *.out *.toc *.synctex.gz

realclean: clean
	rm -f main2017.pdf main2018.pdf main2019.pdf main.bib
