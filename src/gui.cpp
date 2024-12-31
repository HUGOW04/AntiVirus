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
        // Atomic/Biohazard scanning effect
        const float rotationSpeed = time * 1.2f;
        
        // Render biohazard symbol segments
        const int numSymbolSegments = 3;
        float symbolSize = maxRadius * 0.85f;
        float innerRadius = symbolSize * 0.3f;
        float outerRadius = symbolSize * 0.7f;
        
        for (int i = 0; i < numSymbolSegments; i++) {
            float segmentAngle = (2.0f * M_PI * i / numSymbolSegments) + rotationSpeed;
            
            // Draw the main segment arc
            glLineWidth(lineWidth * 1.5f);
            glBegin(GL_LINE_STRIP);
            for (int j = 0; j <= segments / 3; j++) {
                float t = (float)j / (segments / 3);
                float arcAngle = segmentAngle + t * 2.0f * M_PI / numSymbolSegments;
                float scanProgress = (arcAngle + M_PI) / (2.0f * M_PI);
                float alpha = fmod(scanProgress, 1.0f) <= progress ? 1.0f : 0.2f;
                
                // Pulsing effect
                float pulse = sin(time * 5 + t * 10) * 0.1f + 0.9f;
                float radius = outerRadius * pulse;
                
                glColor4f(0.0f, 0.8f, 1.0f, alpha);
                float x = radius * cos(arcAngle);
                float y = radius * sin(arcAngle);
                glVertex2f(x, y);
            }
            glEnd();
            
            // Draw orbital rings
            const int numOrbits = 2;
            for (int orbit = 0; orbit < numOrbits; orbit++) {
                float orbitRadius = innerRadius + (outerRadius - innerRadius) * ((float)orbit / numOrbits);
                float orbitRotation = rotationSpeed * (orbit % 2 ? 1 : -1) * 0.5f;
                
                glLineWidth(lineWidth * 0.8f);
                glBegin(GL_LINE_STRIP);
                for (int j = 0; j <= segments; j++) {
                    float t = (float)j / segments;
                    float angle = segmentAngle + t * 2.0f * M_PI + orbitRotation;
                    float alpha = fmod((angle + M_PI) / (2.0f * M_PI), 1.0f) <= progress ? 0.6f : 0.1f;
                    
                    // Orbital wave effect
                    float wave = sin(angle * 6 + time * 4) * 0.1f;
                    float radius = orbitRadius * (1.0f + wave);
                    
                    glColor4f(0.0f, 1.0f, 1.0f, alpha);
                    float x = radius * cos(angle);
                    float y = radius * sin(angle);
                    glVertex2f(x, y);
                }
                glEnd();
            }
        }
        
        // Electron particles orbiting the structure
        const int numParticles = 18;
        for (int i = 0; i < numParticles; i++) {
            float particleProgress = fmod(time * 0.5f + (float)i / numParticles, 1.0f);
            if (particleProgress <= progress) {
                float angle = particleProgress * 2 * M_PI + rotationSpeed;
                float spiralR = maxRadius * (0.3f + particleProgress * 0.5f);
                
                // Electron core
                glBegin(GL_TRIANGLE_FAN);
                float px = spiralR * cos(angle);
                float py = spiralR * sin(angle);
                
                glColor4f(0.0f, 1.0f, 1.0f, 0.9f);
                glVertex2f(px, py);
                glColor4f(0.0f, 0.8f, 1.0f, 0.0f);
                
                float particleSize = lineWidth * 2.0f;
                for (int j = 0; j <= 12; j++) {
                    float a = j * 2 * M_PI / 12;
                    glVertex2f(px + particleSize * cos(a), py + particleSize * sin(a));
                }
                glEnd();
                
                // Electron trail
                glLineWidth(lineWidth * 0.6f);
                glBegin(GL_LINE_STRIP);
                for (int t = 0; t < 12; t++) {
                    float trailAngle = angle - t * 0.1f;
                    float trailR = spiralR * (1.0f - t * 0.02f);
                    float tx = trailR * cos(trailAngle);
                    float ty = trailR * sin(trailAngle);
                    glColor4f(0.0f, 1.0f, 1.0f, 0.4f * (1.0f - (float)t / 12));
                    glVertex2f(tx, ty);
                }
                glEnd();
            }
        }

        // Central core pulse
        float pulseSize = maxRadius * 0.2f * (1.0f + 0.2f * sin(time * 8));
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.0f, 1.0f, 1.0f, 0.8f);
        glVertex2f(0, 0);
        glColor4f(0.0f, 0.8f, 1.0f, 0.0f);
        
        for (int i = 0; i <= 32; i++) {
            float a = i * 2 * M_PI / 32;
            glVertex2f(pulseSize * cos(a), pulseSize * sin(a));
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