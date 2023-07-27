#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include "olcConsoleGameEngine.h"

class OneLoneCoder_UltimateFPS : public olcConsoleGameEngine
{
public:
    OneLoneCoder_UltimateFPS()
    {
        m_sAppName = L"Ultimate First Person Shooter!";
    }

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
        return true;
    }

    virtual bool OnUserUpdate(float fElapsedTime)
    {
        //player input
        if (m_keys[L'A'].bHeld)
        {
            playerA -= walkSpeed * fElapsedTime;
        }
        if (m_keys[L'D'].bHeld)
        {
            playerA += walkSpeed * fElapsedTime;
        }

        if (m_keys[L'W'].bHeld)
        {
            playerX += sinf(playerA) * 5.0f * fElapsedTime; //shouldn't these be the other way around?
            playerY += cosf(playerA) * 5.0f * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX -= sinf(playerA) * 5.0f * fElapsedTime;
                playerY -= cosf(playerA) * 5.0f * fElapsedTime;
            }
        }

        if (m_keys[L'S'].bHeld)
        {
            playerX -= sinf(playerA) * 5.0f * fElapsedTime;
            playerY -= cosf(playerA) * 5.0f * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX += sinf(playerA) * 5.0f * fElapsedTime;
                playerY += cosf(playerA) * 5.0f * fElapsedTime;
            }
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

            //"march" forward from ray origin untill we hit a wall
            while (!hitWall && distanceToWall < maxDepth)
            {
                distanceToWall += 0.1f;

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

                        //tile boundary check
                        std::vector<std::pair<float, float>> distanceDot;

                        for (int tx = 0; tx < 2; tx++)
                        {
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)tileY + ty - playerY;
                                float vx = (float)tileX + tx - playerX;
                                float magnitude = sqrt(vx * vx + vy * vy);
                                // a_x * b_x + a_y + b_y, keep in mind we use unit vectors
                                float dot = (vectorX * vx / magnitude) + (vectorY * vy / magnitude);
                                distanceDot.push_back(std::make_pair(magnitude, dot));
                            }
                        }

                        std::sort(distanceDot.begin(), distanceDot.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {return left.first < right.first; });

                        //the dot product is already normalized so, cos(o) = a . b,
                        //meaning to get the angle we simply take the arccos of the dot product
                        float bound = 0.01f;
                        if (acos(distanceDot.at(0).second) < bound)
                        {
                            boundary = true;
                        }
                        if (acos(distanceDot.at(1).second) < bound)
                        {
                            boundary = true;
                        }
                        //if (acos(distanceDot.at(2).second) < bound)
                        //{
                        //    boundary = true;
                        //}
                    }
                }
            }

            //calculate the distance from the ceiling to the floor based on raycast distance
            //these points represent for this column where the wall starts, from ceiling to floor
            int ceiling = (float)(ScreenHeight() / 2.0) - ScreenHeight() / ((float)distanceToWall);
            int floor = ScreenHeight() - ceiling;

            short shade = ' ';

            if (distanceToWall <= maxDepth / 4.0f)      shade = 0x2588; //very close
            else if (distanceToWall <= maxDepth / 3.0f) shade = 0x2593;
            else if (distanceToWall <= maxDepth / 2.0f) shade = 0x2592;
            else if (distanceToWall < maxDepth)         shade = 0x2591;
            else                                        shade = ' ';    //too far away

            if (boundary)                               shade = ' ';

            for (int y = 0; y < ScreenHeight(); y++)
            {

                if (y < ceiling) //ceiling
                {
                    //screen[y * ScreenWidth() + x] = ' ';
                    Draw(x, y, L' ');
                }
                else if (y >= ceiling && y <= floor) //wall
                {
                    //screen[y * ScreenWidth() + x] = shade;
                    Draw(x, y, shade);
                }
                else //ground
                {
                    //floor shading (how does this work?)
                    float floorDistance = 1.0f - (((float)y - ScreenHeight() / 2.0f) / ((float)ScreenHeight() / 2.0f));

                    if (floorDistance < 0.25f) shade = '#';
                    else if (floorDistance < 0.5f)  shade = 'x';
                    else if (floorDistance < 0.75f) shade = '.';
                    else if (floorDistance < 0.9f)  shade = '-';
                    else                            shade = ' ';

                    //screen[y * ScreenWidth() + x] = shade;
                    Draw(x, y, shade);
                }
            }
        }
        //...

        //display map
        for (int mx = 0; mx < mapWidth; mx++)
        {
            for (int my = 0; my < mapHeight; my++)
            {
                //screen[(my + 1) * ScreenWidth() + mx] = map[my * mapWidth + mx];
                Draw(mx + 1, my + 1, map[my * mapWidth + mx]);
            }
        }
        //screen[((int)playerY + 1) * screenWidth + (int)playerX] = 'P';
        Draw(1 + (int)playerX, 1 + (int)playerY, L'P');
        //...

        //stats
        //swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", playerX, playerY, playerA, 1.0f / deltaTime);
        //...

        return true;
    }

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
    float walkSpeed = 2.0f;

    std::wstring map;
};

int main()
{
    OneLoneCoder_UltimateFPS game;
    game.ConstructConsole(120, 80, 8, 8);
    game.Start();

    return 0;
}