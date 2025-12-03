///////////////////////////////////////////////
/// 简化版OpenGL点云查看器
/// 不依赖freeglut，使用Windows API和基本OpenGL
/// Copyright(C)2016-2018 Percipio All Rights Reserved
///////////////////////////////////////////////
#include "cloud_viewer.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <algorithm>

#ifdef max 
#undef max
#endif
#ifdef min
#undef min
#endif

// 全局变量
static HDC g_hDC = NULL;
static HGLRC g_hRC = NULL;
static HWND g_hWnd = NULL;
static bool exit_flag = false;

// 点云数据
struct CloudData {
    std::vector<GLfloat> vertices;
    std::vector<GLubyte> colors;
    struct {
        double min[3], max[3];
    } boundingbox;
};

static CloudData g_cloud;
static GLdouble g_trans_center[3] = {0, 0, 0};
static GLdouble g_translate[3] = {0, 0, -100};
static GLfloat g_point_size = 2.0f;
static bool show_help = false;

// 视口和投影
static GLdouble g_fov = 60.0;
static GLint g_win_width = 800;
static GLint g_win_height = 600;

// 鼠标交互
static bool mouse_left_down = false;
static bool mouse_middle_down = false;
static bool mouse_right_down = false;
static int mouse_last_x = 0;
static int mouse_last_y = 0;
static GLdouble rotation_x = 0.0;
static GLdouble rotation_y = 0.0;

// 键盘回调函数指针
static bool(*key_callback)(int) = NULL;

// 窗口过程函数
LRESULT CALLBACK GLWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if (key_callback) {
                // 将虚拟键码转换为ASCII码
                UINT virtualKey = wParam;
                BYTE keyboardState[256];
                GetKeyboardState(keyboardState);
                
                WORD charCode;
                int result = ToAscii(virtualKey, 0, keyboardState, &charCode, 0);
                if (result == 1) {
                    if (key_callback(charCode)) {
                        return 0;
                    }
                }
            }
            
            switch (wParam) {
                case VK_ESCAPE:
                    exit_flag = true;
                    break;
                case 'H':
                    show_help = !show_help;
                    break;
                case VK_SPACE:
                    // 切换点大小
                    g_point_size = (g_point_size == 2.0f) ? 4.0f : 2.0f;
                    break;
                case VK_OEM_PLUS:
                case VK_ADD:
                    rotation_x += 5.0;
                    break;
                case VK_OEM_MINUS:
                case VK_SUBTRACT:
                    rotation_x -= 5.0;
                    break;
            }
            break;
            
        case WM_LBUTTONDOWN:
            mouse_left_down = true;
            mouse_last_x = LOWORD(lParam);
            mouse_last_y = HIWORD(lParam);
            return 0;
            
        case WM_LBUTTONUP:
            mouse_left_down = false;
            return 0;
            
        case WM_MOUSEMOVE:
            if (mouse_left_down) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                
                // 鼠标左键拖拽：旋转
                rotation_y += (x - mouse_last_x) * 0.5;
                rotation_x += (y - mouse_last_y) * 0.5;
                
                mouse_last_x = x;
                mouse_last_y = y;
            }
            return 0;
            
        case WM_RBUTTONDOWN:
            mouse_right_down = true;
            mouse_last_x = LOWORD(lParam);
            mouse_last_y = HIWORD(lParam);
            return 0;
            
        case WM_RBUTTONUP:
            mouse_right_down = false;
            return 0;
            
        case WM_MOUSEWHEEL:
            {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                // 鼠标滚轮：缩放
                g_translate[2] += delta * 0.5;
            }
            return 0;
            
        case WM_MBUTTONDOWN:
            mouse_middle_down = true;
            mouse_last_x = LOWORD(lParam);
            mouse_last_y = HIWORD(lParam);
            return 0;
            
        case WM_MBUTTONUP:
            mouse_middle_down = false;
            return 0;
            
        case WM_SIZE:
            g_win_width = LOWORD(lParam);
            g_win_height = HIWORD(lParam);
            break;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 简单的OpenGL窗口创建函数
BOOL CreateOpenGLWindow(const char* title, int width, int height) {
    printf("[CreateOpenGLWindow] 正在创建OpenGL窗口: %s (%dx%d)\n", title, width, height);
    
    WNDCLASS wc;
    DWORD dwExStyle;
    DWORD dwStyle;
    RECT WindowRect;
    WindowRect.left = (long)0;
    WindowRect.right = (long)width;
    WindowRect.top = (long)0;
    WindowRect.bottom = (long)height;

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)GLWindowProc;  // 使用自定义窗口过程函数
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "OpenGL";

    if (!RegisterClass(&wc)) {
        printf("[CreateOpenGLWindow] 错误：注册窗口类失败，错误代码: %d\n", GetLastError());
        return FALSE;
    }
    printf("[CreateOpenGLWindow] 窗口类注册成功\n");

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW;

    AdjustWindowRect(&WindowRect, dwStyle, FALSE);
    
    g_hWnd = CreateWindowEx(dwExStyle, "OpenGL", title, dwStyle,
                          100, 100, 
                          WindowRect.right - WindowRect.left,
                          WindowRect.bottom - WindowRect.top,
                          NULL, NULL, NULL, NULL);

    if (!g_hWnd) {
        printf("[CreateOpenGLWindow] 错误：创建窗口失败，错误代码: %d\n", GetLastError());
        return FALSE;
    }
    printf("[CreateOpenGLWindow] 窗口创建成功: %p\n", g_hWnd);

    g_hDC = GetDC(g_hWnd);
    if (!g_hDC) {
        printf("[CreateOpenGLWindow] 错误：获取DC失败，错误代码: %d\n", GetLastError());
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
        return FALSE;
    }
    printf("[CreateOpenGLWindow] 获取DC成功: %p\n", g_hDC);

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int iFormat = ChoosePixelFormat(g_hDC, &pfd);
    if (iFormat == 0) {
        printf("[CreateOpenGLWindow] 错误：选择像素格式失败\n");
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
        return FALSE;
    }
    printf("[CreateOpenGLWindow] 像素格式选择成功: %d\n", iFormat);

    if (!SetPixelFormat(g_hDC, iFormat, &pfd)) {
        printf("[CreateOpenGLWindow] 错误：设置像素格式失败，错误代码: %d\n", GetLastError());
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
        return FALSE;
    }
    printf("[CreateOpenGLWindow] 像素格式设置成功\n");

    g_hRC = wglCreateContext(g_hDC);
    if (!g_hRC) {
        printf("[CreateOpenGLWindow] 错误：创建OpenGL上下文失败，错误代码: %d\n", GetLastError());
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
        return FALSE;
    }
    printf("[CreateOpenGLWindow] OpenGL上下文创建成功: %p\n", g_hRC);

    if (!wglMakeCurrent(g_hDC, g_hRC)) {
        printf("[CreateOpenGLWindow] 错误：激活OpenGL上下文失败，错误代码: %d\n", GetLastError());
        wglDeleteContext(g_hRC);
        g_hRC = NULL;
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
        return FALSE;
    }
    printf("[CreateOpenGLWindow] OpenGL上下文激活成功\n");

    // 初始化OpenGL状态
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // 设置默认相机
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLdouble)width / (GLdouble)height, 0.1, 10000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    ShowWindow(g_hWnd, SW_SHOW);
    SetForegroundWindow(g_hWnd);
    SetFocus(g_hWnd);

    return TRUE;
}

// 简单的OpenGL渲染函数
void RenderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 视图变换
    glTranslated(g_translate[0], g_translate[1], g_translate[2]);
    glTranslated(-g_trans_center[0], -g_trans_center[1], -g_trans_center[2]);
    glRotated(rotation_x, 1.0, 0.0, 0.0);
    glRotated(rotation_y, 0.0, 1.0, 0.0);

    // 绘制点云
    if (!g_cloud.vertices.empty()) {
        glPointSize(g_point_size);

        if (!g_cloud.colors.empty()) {
            glEnable(GL_COLOR_ARRAY);
            glColorPointer(3, GL_UNSIGNED_BYTE, 0, g_cloud.colors.data());
            
            glEnableClientState(GL_COLOR_ARRAY);
            glVertexPointer(3, GL_FLOAT, 0, g_cloud.vertices.data());
            
            glDrawArrays(GL_POINTS, 0, g_cloud.vertices.size() / 3);
            
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        } else {
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 0, g_cloud.vertices.data());
            
            glDrawArrays(GL_POINTS, 0, g_cloud.vertices.size() / 3);
            
            glDisableClientState(GL_VERTEX_ARRAY);
        }
    }
    
    // 显示帮助信息
    if (show_help) {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, g_win_width, g_win_height, 0, -1, 1);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2i(10, g_win_height - 20);
        
        // 显示帮助文本（简单版本）
        const char* help = "Controls: ESC-Quit, H-Help, Space-PointSize, +/--Rotate";
        for (const char* p = help; *p; p++) {
            // 使用系统默认字体绘制文本
            // 简化版本，实际应该使用更好的文本渲染
        }
        
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    SwapBuffers(g_hDC);
}

// 改进的消息处理
void ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            exit_flag = true;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// 初始化OpenGL设置
void InitOpenGLSettings(int w, int h) {
    // 设置默认相机
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_fov, (GLdouble)w / (GLdouble)h, 0.1, 10000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // 设置灯光（可选）
    glEnable(GL_LIGHTING);
    GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

int GLPointCloudViewer::GlInit(const char* name, int w, int h) {
    exit_flag = false;
    g_win_width = w;
    g_win_height = h;
    
    // 初始化全局变量
    rotation_x = 0.0;
    rotation_y = 0.0;
    g_translate[0] = 0;
    g_translate[1] = 0;
    g_translate[2] = -100;
    g_point_size = 2.0f;
    show_help = false;
    
    if (!CreateOpenGLWindow(name, w, h)) {
        return -1;
    }
    
    // 初始化OpenGL设置
    InitOpenGLSettings(w, h);
    
    return 0;
}

int GLPointCloudViewer::EnterMainLoop() {
    MSG msg;
    
    while (!exit_flag) {
        // 处理消息
        ProcessMessages();
        
        // 渲染场景
        RenderScene();
        
        // 控制帧率
        Sleep(16); // 约60FPS
    }
    
    return 0;
}

int GLPointCloudViewer::LeaveMainLoop() {
    exit_flag = true;
    return 0;
}

int GLPointCloudViewer::ResetViewTranslate() {
    if (g_cloud.vertices.empty()) {
        return -1;
    }
    
    g_cloud.boundingbox.min[0] = DBL_MAX;
    g_cloud.boundingbox.min[1] = DBL_MAX;
    g_cloud.boundingbox.min[2] = DBL_MAX;
    g_cloud.boundingbox.max[0] = -DBL_MAX;
    g_cloud.boundingbox.max[1] = -DBL_MAX;
    g_cloud.boundingbox.max[2] = -DBL_MAX;

    for (int idx = 0; idx < (int)g_cloud.vertices.size() / 3; idx++) {
        g_cloud.boundingbox.min[0] = std::min(g_cloud.boundingbox.min[0], (GLdouble)g_cloud.vertices[idx * 3]);
        g_cloud.boundingbox.max[0] = std::max(g_cloud.boundingbox.max[0], (GLdouble)g_cloud.vertices[idx * 3]);
        g_cloud.boundingbox.min[1] = std::min(g_cloud.boundingbox.min[1], (GLdouble)g_cloud.vertices[idx * 3 + 1]);
        g_cloud.boundingbox.max[1] = std::max(g_cloud.boundingbox.max[1], (GLdouble)g_cloud.vertices[idx * 3 + 1]);
        g_cloud.boundingbox.min[2] = std::min(g_cloud.boundingbox.min[2], (GLdouble)g_cloud.vertices[idx * 3 + 2]);
        g_cloud.boundingbox.max[2] = std::max(g_cloud.boundingbox.max[2], (GLdouble)g_cloud.vertices[idx * 3 + 2]);
    }

    // 计算中心点和初始位置
    g_trans_center[0] = (g_cloud.boundingbox.max[0] + g_cloud.boundingbox.min[0]) / 2;
    g_trans_center[1] = (g_cloud.boundingbox.max[1] + g_cloud.boundingbox.min[1]) / 2;
    g_trans_center[2] = (g_cloud.boundingbox.max[2] + g_cloud.boundingbox.min[2]) / 2;
    
    // 根据点云大小调整初始距离
    double maxDim = std::max({g_cloud.boundingbox.max[0] - g_cloud.boundingbox.min[0],
                              g_cloud.boundingbox.max[1] - g_cloud.boundingbox.min[1],
                              g_cloud.boundingbox.max[2] - g_cloud.boundingbox.min[2]});
    g_translate[2] = -maxDim * 1.5; // 调整初始距离
    g_translate[0] = g_translate[1] = 0;
    
    // 重置旋转
    rotation_x = 0.0;
    rotation_y = 0.0;
    
    return 0;
}

int GLPointCloudViewer::Update(int point_num, const TY_VECT_3F* points, const uint8_t* color) {
    if (point_num < 0 || !points) {
        printf("[GLPointCloudViewer] Error: Invalid input parameters (point_num=%d, points=%p)\n", point_num, points);
        return -1;
    }

    printf("[GLPointCloudViewer] Start processing %d points\n", point_num);

    g_cloud.vertices.clear();
    g_cloud.colors.clear();
    
    // 先计算有效点的数量，只处理有效的3D点
    int valid_count = 0;
    for (int idx = 0; idx < point_num; idx++) {
        const TY_VECT_3F &p = points[idx];
        if ((!isnan(p.x)) && (!isnan(p.y)) && (!isnan(p.z)) && 
            p.x != 0.0f && p.y != 0.0f && p.z != 0.0f) {
            valid_count++;
        }
    }
    
    if (valid_count == 0) {
        printf("[GLPointCloudViewer] Warning: No valid points to display\n");
        return 0;
    }
    
    printf("[GLPointCloudViewer] Found %d valid points\n", valid_count);
    
    g_cloud.vertices.reserve(valid_count * 3);
    g_cloud.colors.reserve(valid_count * 3);
    
    // 填充有效的点云数据
    for (int idx = 0; idx < point_num; idx++) {
        const TY_VECT_3F &p = points[idx];
        if ((!isnan(p.x)) && (!isnan(p.y)) && (!isnan(p.z)) && 
            p.x != 0.0f && p.y != 0.0f && p.z != 0.0f) {
            
            // 添加顶点数据
            g_cloud.vertices.push_back(p.x);
            g_cloud.vertices.push_back(p.y);
            g_cloud.vertices.push_back(p.z);
            
            // 简化处理：统一使用白色
            g_cloud.colors.push_back(255);
            g_cloud.colors.push_back(255);
            g_cloud.colors.push_back(255);
        }
    }
    
    printf("[GLPointCloudViewer] Successfully updated %d valid points\n", valid_count);
    
    // 调试信息：确认数据是否正确传递给OpenGL渲染
    if (!g_cloud.vertices.empty()) {
        printf("[GLPointCloudViewer] 顶点数量: %d, 颜色数量: %d\n", 
               g_cloud.vertices.size() / 3, g_cloud.colors.size() / 3);
        if (g_cloud.vertices.size() >= 3) {
            printf("[GLPointCloudViewer] 第一个点坐标: (%.2f, %.2f, %.2f)\n",
                   g_cloud.vertices[0], g_cloud.vertices[1], g_cloud.vertices[2]);
        }
    }
    return 0;
}

int GLPointCloudViewer::Deinit() {
    if (g_hRC) {
        wglMakeCurrent(NULL, NULL); // 取消当前OpenGL上下文
        wglDeleteContext(g_hRC);
        g_hRC = NULL;
    }
    if (g_hDC) {
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
    }
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
    return 0;
}

int GLPointCloudViewer::RegisterKeyCallback(bool(*callback)(int)) {
    key_callback = callback;
    return 0;
}