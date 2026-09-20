CXX = g++
WINDRES = windres
CXXFLAGS = -O3 -Wall -std=c++17 -I. -Iimgui -Iinclude -Isrc/core -Isrc/emulation -Isrc/ui -Isrc/platform
LDFLAGS = -Llib -mwindows -lcapstone -ld3d9 -ldwmapi -lcomdlg32 -lgdi32 -luser32 -lole32

TARGET = Ariadne.exe
RC_FILE = resources.rc
RES_FILE = resources.res

SRCS = src/core/cpu_state.cpp \
       src/core/assembler.cpp \
       src/core/disassembler.cpp \
       src/core/editor_document.cpp \
       src/emulation/emulator.cpp \
       src/emulation/debugger.cpp \
       src/ui/theme.cpp \
       src/ui/jump_arrows.cpp \
       src/ui/ui_popups.cpp \
       src/ui/ui_main.cpp \
       src/platform/d3d_device.cpp \
       src/platform/file_dialogs.cpp \
       src/platform/win_main.cpp \
       imgui/imgui.cpp \
       imgui/imgui_draw.cpp \
       imgui/imgui_tables.cpp \
       imgui/imgui_widgets.cpp \
       imgui/imgui_impl_win32.cpp \
       imgui/imgui_impl_dx9.cpp

all: $(TARGET)

$(RES_FILE): $(RC_FILE) icon16.ico icon32.ico
	$(WINDRES) $(RC_FILE) -O coff -o $(RES_FILE)

$(TARGET): $(SRCS) $(RES_FILE)
	$(CXX) $(CXXFLAGS) $(SRCS) $(RES_FILE) -o $(TARGET) $(LDFLAGS)

clean:
	del /f /q $(TARGET) $(RES_FILE)