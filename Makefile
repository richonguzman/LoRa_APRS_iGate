.PHONY: test-digi clean-digi-test

test-digi:
	$(MAKE) -C test/digi-host test

clean-digi-test:
	$(MAKE) -C test/digi-host clean
