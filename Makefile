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

CORE_FILES=Game.cpp InputManager.cpp WindowManager.cpp TimeManager.cpp \
		   OBJFormat.cpp \
		   Model3D.cpp \
		   TextConsole.cpp TrueTypeFont.cpp FreeTypeFont.cpp FontRenderer.cpp \
           Renderer.cpp NOAARenderTarget.cpp MSAARenderTarget.cpp SSAARenderTarget.cpp \
		   FXAARenderTarget.cpp FXAA2RenderTarget.cpp FBRenderTarget.cpp ToonRenderTarget.cpp \
		   Shader.cpp \
		   BlinnPhongShader.cpp ToonLightingShader.cpp NormalShadowMapShader.cpp \
		   Camera.cpp \
		   FlyMotion.cpp FreeFlyMotion.cpp WalkingMotion.cpp

UTILS_FILES=MathUtils.c ImageLoaders.c

OPENGL_FILES=GLFWKeyManager.cpp GLFWMouseManager.cpp GLFWWindowManager.cpp \
			 OpenGLModel3D.cpp \
			 OpenGLFontRenderer.cpp \
			 OpenGLRenderer.cpp OpenGLFilterRenderTarget.cpp \
             OpenGLMSAARenderTarget.cpp OpenGLSSAARenderTarget.cpp OpenGLFBRenderTarget.cpp \
             OpenGLShader.cpp OpenGLShaderMaterial.cpp OpenGLShaderLight.cpp OpenGLUniformBlock.cpp
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
FLAGS=-I/opt/X11/include -I/opt/X11/include/freetype2/ -Wno-deprecated-register
SHAREDGEN= -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0,-install_name,$(LIBNAME)
SHAREDEXT=dylib
PREFIX=/usr/local/lib
else
LDFLAGS= -lGL -lGLEW -lglfw -lfreetype -ljpeg -fPIC
FLAGS=-I/usr/include -I/usr/include/freetype2
SHAREDGEN= -shared
SHAREDEXT=so
PREFIX=/usr/local/lib
endif

#
# Compilation flags
#
FLAGS+=-Werror -MMD -fPIC -Icore/inc -Icore/opengl/inc -Icore/procedural/inc -Iutils/inc -I3rdparty -g -DDEBUG_OPENGL_PIPELINE
CXXFLAGS=$(FLAGS) -std=c++11
CFLAGS=$(FLAGS) -std=c11

#
# Main rules
#
.PHONY: demos

all: engine 

engine: dirs $(LIBDIR)/$(LIBNAME)

demos: engine
	@make -C $(DEMODIR)

dirs:
	@echo "- Creating project dirs...\c"
	@mkdir -p $(OBJDIR)
	@mkdir -p $(LIBDIR)
	@echo "done"

$(LIBDIR)/$(LIBNAME): $(OBJECTS)
	@echo "- Generating $@...\c"
	@$(CXX) $(SHAREDGEN) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "done"

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: %.cpp
	@echo "- Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.c
	@echo "- Compiling $<..."
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "- Cleaning project directories...\c"
	@rm -fr $(OBJDIR)
	@rm -fr $(LIBDIR)
	@make -C $(DEMODIR) clean >/dev/null 2>&1
	@echo "done"
