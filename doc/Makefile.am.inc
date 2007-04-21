# documentation sources
DOCSOURCES = doc/manual.xml doc/ref/zebraimg.xml doc/ref/zebracam.xml \
    doc/version.xml doc/reldate.xml

# man page targets to distribute and install
dist_man_MANS = doc/man/zebraimg.1 doc/man/zebracam.1

# witness to man page build (many-to-many workaround)
man_stamp = doc/man/man.stamp

# TBD add manual content
#dist_doc_DATA = doc/zebra.pdf doc/zebra.html

# distribute all documentation related files to avoid end-user rebuilds
EXTRA_DIST = $(DOCSOURCES) $(man_stamp)

docs: $(dist_man_MANS) #dist_doc_DATA
.PHONY: docs

doc_path = --searchpath $(abs_srcdir)/doc:$(abs_srcdir)/doc/ref
# FIXME xmlto doesnt seem to apply --searchpath to xmllint?
doc_path += --skip-validation

#pdf: doc/zebra-manual.pdf
#doc/zebra-manual.pdf: $(DOCSOURCES)
#	$(XMLTO) $(XMLTOFLAGS) -o doc pdf $<

#html: doc/zebra-manual.html
#doc/zebra-manual.html: $(DOCSOURCES)
#        $(XMLTO) $(XMLTOFLAGS) -o doc xhtml-nochunk $<


$(dist_man_MANS): $(man_stamp)
	@if test ! -f $@ ; then \
	  rm -f $(man_stamp) ; \
	  $(MAKE) $(AM_MAKEFLAGS) $(man_stamp) ; \
	fi

$(man_stamp): $(DOCSOURCES)
	@$(mkdir_p) doc/man 2>/dev/null
	@rm -f $(man_stamp).tmp
	@touch $(man_stamp).tmp
	$(XMLTO) $(doc_path) $(XMLTOFLAGS) -o doc/man man $<
	@mv $(man_stamp).tmp $(man_stamp)
