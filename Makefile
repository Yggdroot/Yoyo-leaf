GOALS   = $(if $(MAKECMDGOALS), $(MAKECMDGOALS), all)
SRC_DIR	= ./src

.PHONY: $(GOALS)

$(GOALS):
	@$(MAKE) -k $@ -C $(SRC_DIR) || exit 1

