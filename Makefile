#
# Roberto Sosa Cano
# Copyright 2013
#

### Project name
PROJECT=engine

### System config
UNAME=$(shell uname)

CXX=g++
CC=gcc

#
# Files to be compiled
#
VPATH=core/src:core/opengl/src:core/procedural/src:utils/src

CORE_FILES=Camera.cpp Game.cpp InputManager.cpp OBJFormat.cpp Renderer.cpp WindowManager.cpp TrueTypeFont.cpp FreeTypeFont.cpp \
		   FontRenderer.cpp NOAARenderTarget.cpp MSAARenderTarget.cpp SSAARenderTarget.cpp TextConsole.cpp FXAARenderTarget.cpp \
		   FXAA2RenderTarget.cpp
UTILS_FILES=MathUtils.c JPEGLoader.c
OPENGL_FILES=GLFWKeyManager.cpp GLFWMouseManager.cpp GLFWWindowManager.cpp \
			 OpenGLNOAARenderTarget.cpp OpenGLMSAARenderTarget.cpp OpenGLSSAARenderTarget.cpp OpenGLFXAARenderTarget.cpp \
             OpenGLRenderer.cpp OpenGLShader.cpp OpenGLObject3D.cpp OpenGLFontRenderer.cpp OpenGLShaderMaterial.cpp \
			 OpenGLUniformBlock.cpp OpenGLFXAA2RenderTarget.cpp
#PROCEDURAL_FILES=Cube.cpp Icosahedron.cpp Plane.cpp Sphere.cpp

FILES=$(CORE_FILES) $(OPENGL_FILES) $(PROCEDURAL_FILES) $(UTILS_FILES)

OBJDIR=obj
CPP_OBJECTS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(FILES))
OBJECTS=$(patsubst %.c,$(OBJDIR)/%.o,$(CPP_OBJECTS))

LIBDIR=lib
LIBNAME=lib$(PROJECT).$(SHAREDEXT)

DEMODIR=demos

# Mac OS alternate cmdline link options
ifeq ($(UNAME), Darwin)
LDFLAGS= -L/opt/X11/lib -lfreetype -lglew -lglfw3 -ljpeg -framework Cocoa -framework OpenGL -framework IOKit -fPIC
CXXFLAGS=-I/opt/X11/include -I/opt/X11/include/freetype2/
SHAREDGEN= -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0,-install_name,$(LIBNAME)
SHAREDEXT=dylib
PREFIX=/usr/local/lib
else
LDFLAGS= -lGL -lGLEW -lglfw -lfreetype -ljpeg -fPIC
CXXFLAGS=-I/usr/include -I/usr/include/freetype2
SHAREDGEN= -shared
SHAREDEXT=so
PREFIX=/usr/local/lib
endif

#
# Compilation flags
#
CXXFLAGS+= -Werror -MMD -fPIC -Icore/inc -Icore/opengl/inc -Icore/procedural/inc -Iutils/inc -I3rdparty -g -DDEBUG_OPENGL_PIPELINE
CFLAGS=$(CXXFLAGS)

#
# Main rules
#
.PHONY: demos

all: engine 

engine: dirs $(LIBDIR)/$(LIBNAME)

install: engine
	@echo "- Installing $(LIBNAME) into $(HOME)/$(LIBDIR)...\c"
	@cp $(LIBDIR)/$(LIBNAME) $(HOME)/$(LIBDIR)/
	@echo "done"

demos: engine install
	@echo "- Compiling project demos...\c"
	@make -C $(DEMODIR) >/dev/null 2>&1
	@echo "done"

dirs:
	@echo "- Creating project dirs...\c"
	@mkdir -p $(OBJDIR)
	@mkdir -p $(LIBDIR)
	@mkdir -p $(HOME)/$(LIBDIR)
	@echo "done"

$(LIBDIR)/$(LIBNAME): $(OBJECTS)
	@echo "- Generating $@...\c"
	@$(CXX) $(SHAREDGEN) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "done"

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: %.cpp
	@echo "- Compiling $<...\c"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo "done"

$(OBJDIR)/%.o: %.c
	@echo "- Compiling $<...\c"
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "done"

clean:
	@echo "- Cleaning project directories...\c"
	@rm -fr $(OBJDIR)
	@rm -fr $(LIBDIR)
	@make -C $(DEMODIR) clean >/dev/null 2>&1
	@echo "done"
