
MAIN = main
BUILD_DIR = build
PDF = $(MAIN).pdf

LATEXMK = latexmk

LATEX_FLAGS = \
	-pdf \
	-interaction=nonstopmode \
	-file-line-error \
	-outdir=$(BUILD_DIR)

.PHONY: all pdf view clean rebuild help

all: pdf

pdf:
	@mkdir -p $(BUILD_DIR)
	@$(LATEXMK) $(LATEX_FLAGS) $(MAIN).tex
	@status=$$?; \
	if [ $$status -eq 0 ] && [ -f "$(BUILD_DIR)/$(PDF)" ]; then \
		cp "$(BUILD_DIR)/$(PDF)" "./$(PDF)"; \
		rm -rf "$(BUILD_DIR)"; \
		echo "PDF generated: $(PDF)"; \
	else \
		echo "PDF generation failed. Check $(BUILD_DIR)/$(MAIN).log"; \
		exit 1; \
	fi

view: pdf
	open $(PDF)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean pdf

help:
	@echo "make         - Generate PDF and clean temporary files"
	@echo "make view    - Generate and open PDF"
	@echo "make clean   - Remove temporary files"
	@echo "make rebuild - Rebuild PDF from scratch"
