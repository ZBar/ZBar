DOCFILES = doc/manual.xml doc/ref/zebraimg.xml doc/ref/zebracam.xml

dist_man_MANS = doc/man/zebraimg.1 doc/man/zebracam.1
#dist_doc_DATA = doc/zebra.pdf doc/zebra.html

docs: $(dist_man_MANS)
.PHONY: docs

doc_path = --searchpath $(srcdir)/doc:$(srcdir)/doc/ref

#pdf: doc/zebra-manual.pdf
#doc/zebra-manual.pdf: $(DOCFILES)
#	$(XMLTO) $(XMLTOFLAGS) -o doc pdf $<

#html: doc/zebra-manual.html
#doc/zebra-manual.html: $(DOCFILES)
#        $(XMLTO) $(XMLTOFLAGS) -o doc xhtml-nochunk $<

man_stamp = doc/man/man.stamp

$(dist_man_MANS): $(man_stamp)
	@if test ! -f $@ ; then \
	  rm -f $(man_stamp) ; \
	  $(MAKE) $(AM_MAKEFLAGS) $(man_stamp) ; \
	fi

$(man_stamp): $(DOCFILES)
	@$(mkdir_p) doc/man 2>/dev/null
	@rm -f $(man_stamp).tmp
	@touch $(man_stamp).tmp
	$(XMLTO) $(doc_path) $(XMLTOFLAGS) -o doc/man man $<
	@mv $(man_stamp).tmp $(man_stamp)
