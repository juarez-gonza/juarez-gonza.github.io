# Makefile for building the blog

.PHONY: build

build:
	@echo "Generating index.html..."
	@python3 gen_index_html.py
	@if [ $$? -eq 0 ]; then \
		echo "Injecting header/footer into posts..."; \
		python3 inject_post_boilerplate.py; \
	else \
		echo "Index generation failed. Skipping post injection."; \
		exit 1; \
	fi

