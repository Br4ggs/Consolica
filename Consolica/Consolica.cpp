#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>

#include "olcConsoleGameEngine.h"

//TODO:
//-fix fisheye effect
//-fix raycasting technique
//-research some of the less understandable math in program
//-floor/ceiling texturing
//-mouse input
//-update sprite code to something more generic

class OneLoneCoder_UltimateFPS : public olcConsoleGameEngine
{
private:
    //int screenWidth = 120;
    //int screenHeight = 40;
    int mapHeight = 16;
    int mapWidth = 16;

    float playerX = 8.0f;
    float playerY = 8.0f;
    float playerA = 0.0f;
    float FOV = 3.14159 / 4.0;
    float maxDepth = 16.0f;
    float walkSpeed = 5.0f;

    std::wstring map;
    olcSprite* spriteWall;
    olcSprite* spriteLamp;
    olcSprite* spriteFireBall;

    float* depthBuffer = nullptr;

    struct Object
    {
        float x;
        float y;
        float velocityX;
        float velocityY;
        bool remove;
        olcSprite* sprite;
    };

    std::list<Object> listObjects;

protected:
    virtual bool OnUserCreate()
    {
        map += L"################";
        map += L"#..............#";
        map += L"#.##...........#";
        map += L"#.#............#";
        map += L"#..............#";
        map += L"#..............#";
        map += L"#...............";
        map += L"#...............";
        map += L"#...............";
        map += L"#..............#";
        map += L"#..............#";
        map += L"#..............#";
        map += L"#...........####";
        map += L"#..............#";
        map += L"#..............#";
        map += L"################";

        spriteWall = new olcSprite(L"../Sprites/fps_wall1.spr");
        spriteLamp = new olcSprite(L"../Sprites/fps_lamp1.spr");
        spriteFireBall = new olcSprite(L"../Sprites/fps_fireball1.spr");

        depthBuffer = new float[ScreenWidth()];

        listObjects = {
            {7, 7, 0.0f, 0.0f, false, spriteLamp}
        };

        return true;
    }

    virtual bool OnUserUpdate(float fElapsedTime)
    {
        //player input
        if (m_keys[L'A'].bHeld)
        {
            playerA -= 1.0f * fElapsedTime;
        }
        if (m_keys[L'D'].bHeld)
        {
            playerA += 1.0f * fElapsedTime;
        }

        if (m_keys[L'W'].bHeld)
        {
            playerX += sinf(playerA) * walkSpeed * fElapsedTime; //shouldn't these be the other way around?
            playerY += cosf(playerA) * walkSpeed * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX -= sinf(playerA) * walkSpeed * fElapsedTime;
                playerY -= cosf(playerA) * walkSpeed * fElapsedTime;
            }
        }

        if (m_keys[L'S'].bHeld)
        {
            playerX -= sinf(playerA) * walkSpeed * fElapsedTime;
            playerY -= cosf(playerA) * walkSpeed * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX += sinf(playerA) * walkSpeed * fElapsedTime;
                playerY += cosf(playerA) * walkSpeed * fElapsedTime;
            }
        }

        //strafing
        if (m_keys[L'E'].bHeld)
        {
            playerX += cosf(playerA) * walkSpeed * fElapsedTime; //shouldn't these be the other way around?
            playerY -= sinf(playerA) * walkSpeed * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX -= cosf(playerA) * walkSpeed * fElapsedTime;
                playerY += sinf(playerA) * walkSpeed * fElapsedTime;
            }
        }

        if (m_keys[L'Q'].bHeld)
        {
            playerX -= cosf(playerA) * walkSpeed * fElapsedTime;
            playerY += sinf(playerA) * walkSpeed * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX += cosf(playerA) * walkSpeed * fElapsedTime;
                playerY -= sinf(playerA) * walkSpeed * fElapsedTime;
            }
        }
        //...

        //fire bullets
        if (m_keys[VK_SPACE].bReleased)
        {
            Object o;
            o.x = playerX;
            o.y = playerY;

            float noise = (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.1f;
            o.velocityX = sinf(playerA + noise) * 8.0f;
            o.velocityY = cosf(playerA + noise) * 8.0f;

            o.sprite = spriteFireBall;
            o.remove = false;
            listObjects.push_back(o);
        }
        //...

        //rendering
        float rayStart = playerA - FOV / 2.0f;
        for (int x = 0; x < ScreenWidth(); x++)
        {
            //this will range between 0 and FOV
            float rayAngleOffset = ((float)x / (float)ScreenWidth()) * FOV;
            float rayAngle = rayStart + rayAngleOffset;

            float distanceToWall = 0;
            bool hitWall = false;
            bool boundary = false;

            float vectorX = std::sinf(rayAngle); //same here?
            float vectorY = std::cosf(rayAngle);

            float sampleX = 0.0f; //sample for texture

            //"march" forward from ray origin untill we hit a wall
            while (!hitWall && distanceToWall < maxDepth)
            {
                distanceToWall += 0.05f;

                //"sample" a position by taking the player pos + the raycast vector
                int tileX = (int)(playerX + vectorX * distanceToWall);
                int tileY = (int)(playerY + vectorY * distanceToWall);

                //test to see sampled tile is not out of bounds
                if (tileX < 0 || tileX >= mapWidth || tileY < 0 || tileY >= mapHeight)
                {
                    hitWall = true;
                    distanceToWall = maxDepth;
                }
                else
                {
                    if (map[tileY * mapWidth + tileX] == '#') //we hit a wall
                    {
                        hitWall = true;

                        float blockMidX = (float)tileX + 0.5f;
                        float blockMidY = (float)tileY + 0.5f;

                        //exact collision points
                        float testPointX = playerX + vectorX * distanceToWall;
                        float testPointY = playerY + vectorY * distanceToWall;

                        float testAngle = atan2((testPointY - blockMidY), (testPointX - blockMidX)); //? needs some more research
                        if (testAngle >= -3.14159f * 0.25f && testAngle < 3.14159f * 0.25f) //check which quadrant we landed in, depends which axis to use for sampling
                        {
                            sampleX = testPointY - (float)tileY;
                        }
                        if (testAngle >= 3.14159f * 0.25f && testAngle < 3.14159f * 0.75f)
                        {
                            sampleX = testPointX - (float)tileX;
                        }
                        if (testAngle < -3.14159f * 0.25f && testAngle >= -3.14159f * 0.75f)
                        {
                            sampleX = testPointX - (float)tileX;
                        }
                        if (testAngle >= 3.14159f * 0.75f || testAngle < -3.14159f * 0.75f)
                        {
                            sampleX = testPointY - (float)tileY;
                        }
                    }
                }
            }

            //calculate the distance from the ceiling to the floor based on raycast distance
            //these points represent for this column where the wall starts, from ceiling to floor
            int ceiling = (float)(ScreenHeight() / 2.0) - ScreenHeight() / ((float)distanceToWall);
            int floor = ScreenHeight() - ceiling;

            //update depth buffer
            depthBuffer[x] = distanceToWall;

            for (int y = 0; y < ScreenHeight(); y++)
            {

                if (y < ceiling) //ceiling
                {
                    Draw(x, y, L' ');
                }
                else if (y >= ceiling && y <= floor) //wall
                {
                    if (distanceToWall < maxDepth)
                    {
                        float sampleY = ((float)y - ceiling) / ((float)floor - (float)ceiling);
                        Draw(x, y, spriteWall->SampleGlyph(sampleX, sampleY), spriteWall->SampleColour(sampleX, sampleY));
                    }
                    else
                    {
                        Draw(x, y, PIXEL_SOLID, 0);
                    }
                }
                else //ground
                {
                    Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN);
                }
            }
        }
        //...

        //Update & Draw Objects
        for (auto& object : listObjects)
        {
            //update object physics
            object.x += object.velocityX * fElapsedTime;
            object.y += object.velocityY * fElapsedTime;

            //check if object is inside wall, if so, set flag for removal
            if (map.c_str()[(int)object.x * mapWidth + (int)object.y] == '#')
            {
                object.remove = true;
            }

            //is object within distance?
            float vecX = object.x - playerX;
            float vecY = object.y - playerY;
            float distanceFromPlayer = sqrt(vecX * vecX + vecY * vecY);

            float eyeX = sinf(playerA);
            float eyeY = cosf(playerA);
            float objectAngle = atan2f(eyeY, eyeX) - atan2f(vecY, vecX);
            if (objectAngle < -3.14159f) //make sure angle lies between 0 and +-2*pi?
            {
                objectAngle += 2.0f * 3.14159f;
            }
            if (objectAngle > 3.14159f)
            {
                objectAngle -= 2.0f * 3.14159f;
            }

            bool inPlayerFOV = fabs(objectAngle) < FOV / 2.0f;

            if (inPlayerFOV && distanceFromPlayer >= 0.5f && distanceFromPlayer < maxDepth)
            {
                float objectCeiling = (float)(ScreenHeight() / 2.0) - ScreenHeight() / ((float)distanceFromPlayer);
                float objectFloor = ScreenHeight() - objectCeiling;
                float objectHeight = objectFloor - objectCeiling;
                float objectAspectRatio = (float)object.sprite->nHeight / (float)object.sprite->nWidth;
                float objectWidth = objectHeight / objectAspectRatio;

                float middleOfObject = (0.5f * (objectAngle / (FOV / 2.0f)) + 0.5f) * (float)ScreenWidth(); //?

                for (float x = 0; x < objectWidth; x++)
                {
                    for (float y = 0; y < objectHeight; y++)
                    {
                        float sampleX = x / objectWidth;
                        float sampleY = y / objectHeight;

                        wchar_t c = object.sprite->SampleGlyph(sampleX, sampleY);
                        int objectColumn = (int)(middleOfObject + x - (objectWidth / 2.0f));
                        if (objectColumn >= 0 && objectColumn < ScreenWidth())
                        {
                            if (c != L' ' && depthBuffer[objectColumn] >= distanceFromPlayer)
                            {
                                Draw(objectColumn, objectCeiling + y, c, object.sprite->SampleColour(sampleX, sampleY));
                                depthBuffer[objectColumn] = distanceFromPlayer;
                            }
                        }
                    }
                }
            }
        }
        //...

        //remove dead objects from object list
        listObjects.remove_if([](Object& o) {return o.remove; });

        //display map
        for (int mx = 0; mx < mapWidth; mx++)
        {
            for (int my = 0; my < mapHeight; my++)
            {
                Draw(mx + 1, my + 1, map[my * mapWidth + mx]);
            }
        }
        Draw(1 + (int)playerX, 1 + (int)playerY, L'P');
        //...

        //stats
        //swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", playerX, playerY, playerA, 1.0f / deltaTime);
        //...

        return true;
    }

public:
    OneLoneCoder_UltimateFPS()
    {
        m_sAppName = L"Ultimate First Person Shooter!";
    }
};

int main()
{
    OneLoneCoder_UltimateFPS game;
    game.ConstructConsole(320, 240, 4, 4);
    game.Start();

    return 0;
}