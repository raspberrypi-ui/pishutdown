bin_PROGRAMS = lxde-pi-shutdown-helper

lxde_pi_shutdown_helper_CFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)/pishutdown"\" \
	-DPACKAGE_BIN_DIR=\""$(bindir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(PACKAGE_CFLAGS) \
	$(G_CAST_CHECKS)

lxde_pi_shutdown_helper_SOURCES = pishutdown.c

lxde_pi_shutdown_helper_includedir = $(includedir)/pishutdown

lxde_pi_shutdown_helper_include_HEADERS =

lxde_pi_shutdown_helper_LDFLAGS = $(DYNAMIC_FLAGS)
lxde_pi_shutdown_helper_DEPENDENCIES_EXTRA = $(BUILTIN_PLUGINS)

lxde_pi_shutdown_helper_LDADD = \
		$(BUILTIN_PLUGINS) \
		$(PACKAGE_LIBS) \
		$(X11_LIBS) \
		$(INTLLIBS)

EXTRA_DIST =
