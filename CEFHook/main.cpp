#pragma once

#include <windows.h>
#include <stdio.h>
#include "include\cef_base.h"
#include "torque.h"

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/cef_base.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"

#include <stdlib.h>
#include "glapi.h"

#include "MologieDetours/detours.h"

#include <gl\GL.h>
#include "glext.h"

#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")

#define CEFHOOK_VERS_MAJ "0"
#define CEFHOOK_VERS_MIN "0"
#define CEFHOOK_VERS_REV "1"

HANDLE thread;
HANDLE blockland;
HANDLE event;

struct TextureObject {
	TextureObject *next;
	TextureObject *prev;
	TextureObject *hashNext;
	unsigned int texGLName;
	unsigned int smallTexGLName;
	const char *texFileName;
	DWORD *type_GBitmap_bitmap;
	unsigned int texWidth;
	unsigned int texHeight;
	unsigned int bitmapWidth;
	unsigned int bitmapHeight;
	unsigned int downloadedWidth;
	unsigned int downloadedHeight;
	unsigned int enum_TextureHandleType_type;
	bool filterNearest;
	bool clamp;
	bool holding;
	int refCount;
};

CefRefPtr<CefBrowser> BLBrowserInstance;

typedef int(*swapBuffersFn)();
static int texID = 0; //Texture ID that we bind to with OpenGL

static bool dirty = false; //Do we need to render the texture?

static int global_ww = 1024; //Width
static int global_hh = 768; //Height, both are used in swapBuffers to determine what should be copied over.

MologieDetours::Detour<swapBuffersFn>* swapBuffers_detour; //The detour so we can draw our stuff before Torque can.

GLuint* texBuffer;

int __fastcall swapBuffers_hook() {
	if (texID != 0 && dirty) {
		BL_glBindTexture(GL_TEXTURE_2D, texID);
		BL_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, global_ww, global_hh, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texBuffer);

		BL_glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		dirty = false;
	}

	int ret = swapBuffers_detour->GetOriginalFunction()();

	return ret;
}

class BLBrowser : public CefApp {

	public:

		BLBrowser() {
			
		};
		// CefBrowserProcessHandler methods:
		//rtual void OnRegisterCustomSchemes() OVERRIDE;

	private:
		// Include the default reference counting implementation.
		IMPLEMENT_REFCOUNTING(BLBrowser);

};

class BLBrowserRenderer : public CefRenderHandler {

	public:

		BLBrowserRenderer(int w, int h) : height(h), width(w) {
			if (!BL_glGenBuffers) {
				Printf("Could not find genBuffers!");

				if (!BL_glGenBuffersARB) {
					Printf("Could not find BL_genBuffersArb!");
					texBuffer = (GLuint*)malloc(2048 * 2048 * 4);
					memset((void*)texBuffer, 0, 2048 * 2048 * 4);
				} else {
					BL_glGenBuffersARB(1, &*texBuffer);
					BL_glBindBufferARB(GL_TEXTURE_BUFFER, *texBuffer);
				}
			} else {

			}
		};

		~BLBrowserRenderer() {
			if (BL_glDeleteBuffersARB) {
				BL_glDeleteBuffersARB(1, &*texBuffer);
			} else {
				delete texBuffer;
			}
		};

		CefRefPtr<CefAccessibilityHandler> GetAccessibilityHandler() {
			return nullptr;
		}

		bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
			rect = CefRect(0, 0, width, height);
			return true;
		}

		bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
			rect = CefRect(0, 0, width, height);
			return true;
		}

		bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) {
			screen_info.rect = CefRect(0, 0, width, height);
			screen_info.device_scale_factor = 1.0;
			return true;
		}

		void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, CefRenderHandler::CursorType type, const CefCursorInfo& custom_cursor_info) {

		}

		void OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser, const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds) {

		}

		void OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int w, int h) {
			if (texID != 0) {
				if (BL_glBindBufferARB) {
					BL_glBindBufferARB(GL_TEXTURE_BUFFER, *texBuffer);
					BL_glBufferDataARB(GL_TEXTURE_BUFFER, width * height * 4, buffer, GL_DYNAMIC_DRAW);
				} else {
					memcpy(texBuffer, buffer, w * h * 4);
				}

				dirty = true;
			}
		}

		void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y) {

		}

		bool StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr< CefDragData > drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y) {
			return false;
		}

		void UpdateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) {

		}

		void UpdateResolution(int hh, int ww) {
			if (hh * ww * 4 > 16777215) {
				Printf("That's too damn big.");
				return;
			}
		
			memset(texBuffer, 0, 2048 * 2048 * 4);
			height = hh;
			width = ww;
			global_ww = width;
			global_hh = height;
		}

	private:

		IMPLEMENT_REFCOUNTING(BLBrowserRenderer);
		int height, width;

};

CefRefPtr<BLBrowserRenderer> renderHandler;

class BLBrowserClient : public CefClient, public CefLifeSpanHandler, public CefLoadHandler {

	public:

		BLBrowserClient(CefRefPtr<CefRenderHandler> ptr) : renderHandler(ptr) {

		}

		virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() {
			return this;
		}

		virtual CefRefPtr<CefLoadHandler> GetLoadHandler() {
			return this;
		}

		virtual CefRefPtr<CefRenderHandler> GetRenderHandler() {
			return renderHandler;
		}

		// CefLifeSpanHandler methods.
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) {
			// Must be executed on the UI thread.
			//CEF_REQUIRE_UI_THREAD();

			browser_id = browser->GetIdentifier();
		}

		bool DoClose(CefRefPtr<CefBrowser> browser) {
			// Must be executed on the UI thread.
			//CEF_REQUIRE_UI_THREAD();

			// Closing the main window requires special handling. See the DoClose()
			// documentation in the CEF header for a detailed description of this
			// process.
			if (browser->GetIdentifier() == browser_id)
			{
				// Set a flag to indicate that the window close should be allowed.
				closing = true;
			}

			// Allow the close. For windowed browsers this will result in the OS close
			// event being sent.
			return false;
		}

		void OnBeforeClose(CefRefPtr<CefBrowser> browser) {

		}

		// Popup mitigation.
		bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name, WindowOpenDisposition target_disposition, bool user_gesture, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, CefBrowserSettings& settings, bool* no_javascript_access) {
			//if (browser->GetMainFrame()->GetIdentifier() == frame->GetIdentifier())
			//	frame->LoadURL(target_url);

			return true;
		}

		void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
			loaded = true;
		}

		bool OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString & failedUrl, CefString & errorText) {
			loaded = true;
		}

		void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {

		}

		void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) {

		}

		bool closeAllowed() const {
			return closing;
		}

		bool isLoaded() const {
			return loaded;
		}

	private:

		int browser_id;
		bool closing = false;
		bool loaded = false;
		CefRefPtr<CefRenderHandler> renderHandler;

		IMPLEMENT_REFCOUNTING(BLBrowserClient);

};

bool bindTexID() {
	TextureObject* texture;
	const char* string = "Add-Ons/Print_Screen_Cinema/prints/Cinema.png";
	texID = 0;

	for (texture = (TextureObject*)0x7868E0; texture; texture = texture->next) {
		if (texture->texFileName != NULL && _stricmp(texture->texFileName, string) == 0) {
			texID = texture->texGLName;
			Printf("Found textureID: %u", texture->texGLName);
			return true;
		}
	}

	Printf("TextureID not found.");

	return false;
}

bool ts_bindTexture(SimObject* this_, int argc, const char* argv[]) {
	return bindTexID();
}

void ts_goToURL(SimObject* this_, int argc, const char* argv[]) {
	if (BLBrowserInstance.get() != nullptr) {
		BLBrowserInstance->GetMainFrame()->LoadURL(CefString(argv[1]));
	} else {
		Printf("BL Browser instance does not exist!");
	}
}

void ts_resizeWindow(SimObject* this_, int argc, const char* argv[]) {
	int width = atoi(argv[1]);
	int height = atoi(argv[2]);

	renderHandler->UpdateResolution(width, height);

	BLBrowserInstance->GetHost()->WasResized();
}

void ts_mouseMove(SimObject* this_, int argc, const char* argv[]) {
	CefMouseEvent* mouseEvent = new CefMouseEvent();
	mouseEvent->x = atoi(argv[1]);
	mouseEvent->y = atoi(argv[2]);
	BLBrowserInstance->GetHost()->SendMouseMoveEvent(*mouseEvent, false);

	delete mouseEvent;
}

void ts_mouseClick(SimObject* this_, int argc, const char* argv[]) {
	CefMouseEvent* mouseEvent = new CefMouseEvent();
	mouseEvent->x = atoi(argv[1]);
	mouseEvent->y = atoi(argv[2]);
	
	int clickType = atoi(argv[3]);
	BLBrowserInstance->GetHost()->SendMouseClickEvent(*mouseEvent, (cef_mouse_button_type_t)clickType, false, 1);
	BLBrowserInstance->GetHost()->SendMouseClickEvent(*mouseEvent, (cef_mouse_button_type_t)clickType, true, 1);

	delete mouseEvent;
}

void ts_mouseWheel(SimObject* this_, int argc, const char* argv[]) {
	CefMouseEvent* mouseEvent = new CefMouseEvent();
	mouseEvent->x = atoi(argv[1]);
	mouseEvent->y = atoi(argv[2]);

	int deltaX = atoi(argv[3]);
	int deltaY = atoi(argv[4]);
	BLBrowserInstance->GetHost()->SendMouseWheelEvent(*mouseEvent, deltaX, deltaY);

	delete mouseEvent;
}

void ts_keyboardEvent(SimObject* this_, int argc, const char* argv[]) {
	CefKeyEvent* keyEvent = new CefKeyEvent();
	keyEvent->character = argv[1][0];
	keyEvent->modifiers = atoi(argv[2]);
	BLBrowserInstance->GetHost()->SendKeyEvent(*keyEvent);

	delete keyEvent;
}

bool* cefRunning = new bool(true);

DWORD WINAPI mainLoop(LPVOID lpParam) {
	CefMainArgs args;

	CefSettings settings;
	settings.single_process = false;
	settings.command_line_args_disabled = true;
	settings.no_sandbox = true;
	settings.windowless_rendering_enabled = true;
	settings.log_severity = cef_log_severity_t::LOGSEVERITY_WARNING;
	//CefString(&settings.resources_dir_path).FromASCII("./CEF");
	//CefString(&settings.locales_dir_path).FromASCII("./CEF/locales");
	//CefString(&settings.log_file).FromASCII("./CEF/debug.log");
	CefString(&settings.browser_subprocess_path).FromASCII("BlocklandCEFSubProcess.exe");
	CefString(&settings.user_agent).FromASCII("Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.146 Safari/537.36 Blockland/r1997 (Torque Game Engine/1.3)");

	if (!CefInitialize(args, settings, new BLBrowser(), nullptr)) {
		Printf("Failed to initialize CEF!");
		return false;
	} else {
		renderHandler = new BLBrowserRenderer(global_ww, global_hh);
		CefBrowserSettings browser_settings;
		CefWindowInfo window_info;
		CefRefPtr<BLBrowserClient> browserClient;
		browserClient = new BLBrowserClient(renderHandler);
		browser_settings.background_color = CefColorSetARGB(255, 255, 255, 255);
		browser_settings.windowless_frame_rate = 60;
		window_info.SetAsWindowless(0);
		BLBrowserInstance = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), "about:blank", browser_settings, NULL);

		while (*cefRunning) {
			CefDoMessageLoopWork();

			Sleep(1);
		}
	}

	BLBrowserInstance->GetHost()->CloseBrowser(true);
	CefDoMessageLoopWork();
	CefDoMessageLoopWork();
	CefDoMessageLoopWork();
	CefShutdown();

	return false;
}

bool init() {
	if (!torque_init())
		return false;

	ConsoleFunction(NULL, "CEF_bindTexture", ts_bindTexture, "() - Bind to the texture representing a CEF screen.", 1, 1);
	ConsoleFunction(NULL, "CEF_goToURL", ts_goToURL, "(string url) - Visit a URL.", 2, 2);
	ConsoleFunction(NULL, "CEF_resizeWindow", ts_resizeWindow, "(int width, int height) - Resize the CEF window, reallocating the texture buffer.", 3, 3);

	ConsoleFunction(NULL, "CEF_mouseMove", ts_mouseMove, "(int x, int y) - Move the mouse to this position.", 3, 3);
	ConsoleFunction(NULL, "CEF_mouseClick", ts_mouseClick, "(int x, int y, int clickType) - Send a click event on the specified coordinates.", 4, 4);
	ConsoleFunction(NULL, "CEF_mouseWheel", ts_mouseWheel, "(int x, int y, int deltaX, int deltaY) - Send a mousewheel event at the coords.", 5, 5);
	ConsoleFunction(NULL, "CEF_keyboardEvent", ts_keyboardEvent, "(char key, int modifiers) - Send a keyboard event to CEF.", 3, 3);

	ConsoleFunction(NULL, "clientCmdCEF_goToURL", ts_goToURL, "(string url) - Visit a URL.", 2, 2);
	ConsoleFunction(NULL, "clientCmdCEF_mouseMove", ts_mouseMove, "(int x, int y) - Move the mouse to this position.", 3, 3);
	ConsoleFunction(NULL, "clientCmdCEF_mouseClick", ts_mouseClick, "(int x, int y, int clickType) - Send a click event on the specified coordinates.", 4, 4);
	ConsoleFunction(NULL, "clientCmdCEF_mouseWheel", ts_mouseWheel, "(int x, int y, int deltaX, int deltaY) - Send a mousewheel event at the coords.", 5, 5);

	SetGlobalVariable("CEFHOOK::MIN", CEFHOOK_VERS_MIN);
	SetGlobalVariable("CEFHOOK::MAJ", CEFHOOK_VERS_MAJ);
	SetGlobalVariable("CEFHOOK::REV", CEFHOOK_VERS_REV);

	Eval("package CEFPackage{function clientCmdMissionStartPhase3(%a0, %a1, %a2){Parent::clientCmdMissionStartPhase3(%a0, %a1, %a2);CEF_bindTexture();}function flushTextureCache(){Parent::flushTextureCache();schedule(500,0,CEF_bindTexture);}function disconnect(%r){CEF_goToURL(\"about:blank\");Parent::disconnect(%r);} function optionsDlg::applyGraphics(%this) { parent::applyGraphics(%this); schedule(500,ServerConnection,CEF_bindTexture); } }; activatePackage(\"CEFPackage\");");
	Eval("function clientCmdCEF_Version(){commandToServer('CEF_Version', $CEFHOOK::MAJ, $CEFHOOK::MIN, $CEFHOOK::REV);}");
	
	initGL();
	swapBuffers_detour = new MologieDetours::Detour<swapBuffersFn>((swapBuffersFn)0x4237D0, (swapBuffersFn)swapBuffers_hook);

	blockland = GetCurrentThread();
	event = CreateEvent(NULL, TRUE, FALSE, "blcefevent");
	thread = CreateThread(NULL, 0, mainLoop, 0, 0, NULL);

	return true;
}

bool deinit() {
	if (*cefRunning) {
		*cefRunning = false;

		WaitForSingleObject(event, 1000);
		TerminateThread(thread, 0);
		CloseHandle(thread);
		CloseHandle(event);
	}

	if (swapBuffers_detour != NULL)
		delete swapBuffers_detour;

	free(texBuffer);

	delete cefRunning;

	return true;
}

int WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			return init();
		case DLL_PROCESS_DETACH:
			return deinit();
		default:
			return true;
	}
}