# documentation sources
DOCSOURCES = doc/manual.xml doc/version.xml doc/reldate.xml \
    doc/ref/zbarimg.xml doc/ref/zbarcam.xml doc/ref/commonoptions.xml


# man page targets to distribute and install
dist_man_MANS =
if HAVE_MAGICK
dist_man_MANS += doc/man/zbarimg.1
endif
if HAVE_VIDEO
dist_man_MANS += doc/man/zbarcam.1
endif

# witness to man page build (many-to-many workaround)
man_stamp = doc/man/man.stamp

# TBD add manual content
#dist_doc_DATA = doc/zbar.pdf doc/zbar.html

# distribute all documentation related files to avoid end-user rebuilds
EXTRA_DIST += $(DOCSOURCES) $(man_stamp)
EXTRA_DIST += doc/api/footer.html doc/style.xsl

docs: $(dist_man_MANS) #dist_doc_DATA
.PHONY: docs

doc_path = --searchpath $(abs_builddir)/doc -m $(abs_srcdir)/doc/style.xsl
# xmlto --searchpath broken again...
doc_path += --skip-validation

#pdf: doc/zbar-manual.pdf
#doc/zbar-manual.pdf: $(DOCSOURCES)
#	$(XMLTO) $(XMLTOFLAGS) -o doc pdf $<

html-local: doc/html/index.html
doc/html/index.html: $(DOCSOURCES)
	$(XMLTO) $(doc_path) $(XMLTOFLAGS) -o doc/html xhtml $<


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
