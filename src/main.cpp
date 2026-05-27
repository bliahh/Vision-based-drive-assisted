#include <GL/glew.h>
#include <GL/freeglut.h>
#include "hud_renderer.h"
#include "frame_data.h"
#include "tcp_thread.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <cstdlib>

#ifndef ASSET_DIR
#define ASSET_DIR ".."
#endif

static std::mutex g_frame_mutex;
static FrameData g_current_frame;
static std::atomic<bool> g_app_running{true};
static HudRenderer g_renderer;
static bool g_gl_initialized = false;


void display() {

    if (!g_gl_initialized) {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) return;
        g_renderer.init(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), ASSET_DIR);
        g_gl_initialized = true;
        std::cout << "[OK] OpenGL " << glGetString(GL_VERSION) << "\n";
    }

    FrameData frame_snapshot;

    {
        std::lock_guard<std::mutex> lock(g_frame_mutex);
        frame_snapshot = g_current_frame;
    }

    glClearColor(0.02f, 0.02f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_renderer.drawSky();
    glDisable(GL_DEPTH_TEST);
    g_renderer.drawStars();
    glEnable(GL_DEPTH_TEST);
    g_renderer.drawRain();
    g_renderer.drawRoad();
    g_renderer.drawTrotuar();
    g_renderer.drawDrivableArea(frame_snapshot.drivable_area);
    g_renderer.drawEgoCorridor(frame_snapshot.lane_lines, frame_snapshot.drivable_area);
    g_renderer.drawCars(frame_snapshot.detected_cars);
    g_renderer.drawSigns(frame_snapshot.detected_signs);
    g_renderer.drawEgoCar();
    g_renderer.drawBuildings();
    g_renderer.drawPersons(frame_snapshot.detected_cars);
    g_renderer.drawStreetLights();

    glutSwapBuffers();
    glutPostRedisplay();
}


void reshape(int w, int h) {

    glViewport(0, 0, w, h);
    g_renderer.resize(w, h);
}


void keyboard(unsigned char key, int x, int y) {

    switch (key) {
        case 'f':
            g_renderer.fog_enabled = !g_renderer.fog_enabled;
            printf("[FOG] %s\n", g_renderer.fog_enabled ? "ON" : "OFF");
            break;
        case '+':
            g_renderer.fog_density += 0.01f;
            printf("[FOG] density=%.3f\n", g_renderer.fog_density);
            break;
        case '-':
            g_renderer.fog_density -= 0.01f;
            if (g_renderer.fog_density < 0.0f) g_renderer.fog_density = 0.0f;
            printf("[FOG] density=%.3f\n", g_renderer.fog_density);
            break;
        case 'r':
            g_renderer.rain_enabled = !g_renderer.rain_enabled;
            printf("[RAIN] %s\n", g_renderer.rain_enabled ? "ON" : "OFF");
            break;
    }
}


int main(int argc, char** argv) {

    int tcp_port = 5005;

    for (int i = 1; i < argc - 1; i++) {
        if (std::string(argv[i]) == "--port") {
            tcp_port = std::atoi(argv[i + 1]);
        }
    }

    std::thread tcp_receiver_thread(
        runTcpReceiverThread,
        tcp_port,
        std::ref(g_current_frame),
        std::ref(g_frame_mutex),
        std::ref(g_app_running)
    );

    std::cout << "[INFO] Port TCP: " << tcp_port << "\n";

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Vision Drive Assist");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    g_app_running = false;
    if (tcp_receiver_thread.joinable()) tcp_receiver_thread.join();

    return 0;
}
