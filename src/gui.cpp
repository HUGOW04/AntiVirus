#include "gui.h"
#include "scan.h"
#include "callback.h"

float colorOffset = 0.0f;

void renderDynamicProgressAnimation(float centerX, float centerY, float size, float progress, float time, bool isScanning) {
const int segments = 120;
const float lineWidth = 4.0f;
const float maxRadius = size / 2;

glPushMatrix();
glTranslatef(centerX, centerY, 0);

if (isScanning) {
    const float rotationSpeed = time * 1.2f;
    
    // Core star effect
    float coreSize = maxRadius * 0.25f;
    float corePulse = sin(time * 12) * 0.2f + 1.0f;
    
    // Diagonal ring 2 (top-right to bottom-left) with wave pattern
    glPushMatrix();
    glRotatef(-45, 0, 0, 1);
    glScalef(1, 0.4, 1);
    
    glLineWidth(lineWidth * 1.2f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        float angle = t * 2 * M_PI - time * 2;
        float wave = sin(t * 12 + time * 8) * 0.2f;
        float radius = maxRadius * 0.8f * (1.0f + wave);
        
        float alpha = 0.6f + 0.4f * sin(t * 6 + time * 4);
        glColor4f(0.0f, 0.8f, 1.0f, alpha);
        glVertex2f(radius * cos(angle), radius * sin(angle));
    }
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glRotatef(45, 0, 0, 1);
    glScalef(1, 0.4, 1);
    
    glLineWidth(lineWidth * 1.2f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        float angle = t * 2 * M_PI - time * 2;
        float wave = sin(t * 12 + time * 8) * 0.2f;
        float radius = maxRadius * 0.8f * (1.0f + wave);
        
        float alpha = 0.6f + 0.4f * sin(t * 6 + time * 4);
        glColor4f(0.0f, 0.8f, 1.0f, alpha);
        glVertex2f(radius * cos(angle), radius * sin(angle));
    }
    glEnd();
    glPopMatrix();
    
    // Core star
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 1.0f, 1.0f, 0.9f);
    glVertex2f(0, 0);
    glColor4f(0.0f, 0.4f, 1.0f, 0.0f);
    
    for (int i = 0; i <= 32; i++) {
        float angle = i * 2 * M_PI / 32;
        float radius = coreSize * corePulse;
        float spike = sin(angle * 6 + time * 8) * 0.3f;
        radius *= (1.0f + spike);
        glVertex2f(radius * cos(angle), radius * sin(angle));
    }
    glEnd();
        
    } else {
        // Existing idle animation remains unchanged
        const int numOrbitals = 3;
        const int particlesPerOrbit = 6;
        
        for (int orbit = 0; orbit < numOrbitals; orbit++) {
            float orbitRadius = maxRadius * (0.4f + orbit * 0.3f);
            float orbitSpeed = 0.7f - orbit * 0.2f;
            
            for (int i = 0; i < particlesPerOrbit; i++) {
                float baseAngle = (2 * M_PI * i) / particlesPerOrbit;
                float angle = baseAngle + time * orbitSpeed * (orbit % 2 == 0 ? 1 : -1);
                
                float wobble = sin(time * 3 + baseAngle) * 0.1f;
                float x = orbitRadius * (1 + wobble) * cos(angle);
                float y = orbitRadius * (1 + wobble) * sin(angle);
                
                glLineWidth(lineWidth * 0.5f);
                glBegin(GL_LINE_STRIP);
                for (int t = 0; t < 8; t++) {
                    float trailAngle = angle - t * 0.1f * (orbit % 2 == 0 ? 1 : -1);
                    float trailX = orbitRadius * cos(trailAngle);
                    float trailY = orbitRadius * sin(trailAngle);
                    float trailAlpha = 0.3f * (1.0f - (float)t / 8);
                    glColor4f(0.0f, 1.0f, 1.0f, trailAlpha);
                    glVertex2f(trailX, trailY);
                }
                glEnd();
                
                float particleSize = lineWidth * (1.0f - orbit * 0.2f);
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(0.0f, 1.0f, 1.0f, 0.8f);
                glVertex2f(x, y);
                glColor4f(0.0f, 1.0f, 1.0f, 0.0f);
                for (int j = 0; j <= 16; j++) {
                    float a = j * 2 * M_PI / 16;
                    glVertex2f(x + particleSize * cos(a), y + particleSize * sin(a));
                }
                glEnd();
            }
        }
    }

    glPopMatrix();
}

// Function to render text
void renderText(const char* text, float x, float y) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}



// Function to render the diagonal gradient background
void renderBackground() {
    float backgroundStartColor[3];
    float backgroundEndColor[3];

    // Change colors based on scanning state
    if (scanning) {
        backgroundStartColor[0] = 0.2f; backgroundStartColor[1] = 0.0f; backgroundStartColor[2] = 0.0f;  // Dark red
        backgroundEndColor[0] = 1.0f; backgroundEndColor[1] = 0.5f; backgroundEndColor[2] = 0.0f;        // Orange
    } else {
        backgroundStartColor[0] = 0.0f; backgroundStartColor[1] = 0.0f; backgroundStartColor[2] = 0.2f;  // Dark blue
        backgroundEndColor[0] = 0.0f; backgroundEndColor[1] = 0.5f; backgroundEndColor[2] = 1.0f;        // Light blue
    }
    glBegin(GL_TRIANGLES);
    
    // Bottom-left to top-right diagonal
    for (int i = 0; i <= WIDTH + HEIGHT; i += 10) {
        float t1 = (float)i / (WIDTH + HEIGHT);
        float t2 = (float)(i + 10) / (WIDTH + HEIGHT);
        
        float wave1 = (sin(colorOffset + t1 * 3.14159f) + 1.0f) / 2.0f;
        float wave2 = (sin(colorOffset + t2 * 3.14159f) + 1.0f) / 2.0f;
        
        float r1 = backgroundStartColor[0] * (1 - wave1) + backgroundEndColor[0] * wave1;
        float g1 = backgroundStartColor[1] * (1 - wave1) + backgroundEndColor[1] * wave1;
        float b1 = backgroundStartColor[2] * (1 - wave1) + backgroundEndColor[2] * wave1;
        
        float r2 = backgroundStartColor[0] * (1 - wave2) + backgroundEndColor[0] * wave2;
        float g2 = backgroundStartColor[1] * (1 - wave2) + backgroundEndColor[1] * wave2;
        float b2 = backgroundStartColor[2] * (1 - wave2) + backgroundEndColor[2] * wave2;
        
        glColor3f(r1, g1, b1);
        glVertex2f(0, i);
        glVertex2f(i, 0);
        
        glColor3f(r2, g2, b2);
        glVertex2f(10, i + 10);
        
        glColor3f(r1, g1, b1);
        glVertex2f(i, 0);
        glVertex2f(i + 10, 0);
        
        glColor3f(r2, g2, b2);
        glVertex2f(10, i + 10);
    }
    
    glEnd();
}
