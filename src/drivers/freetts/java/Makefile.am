SUBDIRS = org
if BUILD_FREETTS_DRIVER
FREETTS_DRIVER_CLASSES= \
	org/GNOME/Speech/*.class
CLEANFILES = $(FREETTS_DRIVER_CLASSES)
CLEANFILES+=freetts-synthesis-driver.jar

$(FREETTS_DRIVER_CLASSES): $(srcdir)/org/GNOME/Speech/*.java
	$(JAVAC) -d . \
	-classpath $(top_builddir)/java/gnome-speech.jar:$(GNOME_SPEECH_CLASSPATH):$(FREETTS_CLASSPATH) \
	$(srcdir)/org/GNOME/Speech/*.java

freetts-synthesis-driver.jar: $(FREETTS_DRIVER_CLASSES)
	$(JAR) -cfm freetts-synthesis-driver.jar \
	$(srcdir)/org/GNOME/Speech/manifest.mf \
	$(FREETTS_DRIVER_CLASSES)

freetts_driver_jardir = \
	$(prefix)/share/jar

freetts_driver_jar_DATA = \
	freetts-synthesis-driver.jar

endif
