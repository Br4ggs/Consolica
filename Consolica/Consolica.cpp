#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>

#include <Windows.h>

#include "olcConsoleGameEngine.h"

//TODO:
//-redo object rendering
//-update sprite code to something more generic (use png image format)
//-research some of the less understandable math in program
//-scale down tile size for a higher resolution map

void sortSprites(std::vector<int> &order, std::vector<float> &distance, int count)
{
    std::vector<std::pair<double, int>> sprites(count);
    for (int i = 0; i < count; i++)
    {
        sprites[i].first = distance[i];
        sprites[i].second = order[i];
    }

    std::sort(sprites.begin(), sprites.end());

    for (int i = 0; i < count; i++)
    {
        distance[i] = sprites[count - i - 1].first;
        order[i] = sprites[count - i - 1].second;
    }
}

class OneLoneCoder_UltimateFPS : public olcConsoleGameEngine
{
private:
    int mapHeight = 16;
    int mapWidth = 16;

    float playerX = 8.5f;
    float playerY = 7.5f;
    float playerA = 0.0f;
    float FOV = 3.14159 / 4.0;
    float maxDepth = 12.0f;
    float walkSpeed = 5.0f;

    int oldMousePos = 0;

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

    std::vector<Object> objects;

    bool running = true;

    //required for keeping cursor in center of window
    HWND window;
    RECT rect;
    POINT center;

protected:
    virtual bool OnUserCreate()
    {
        //get currently active forground window (the console) and center it on screen
        window = GetForegroundWindow();

        SetCapture(window);
        GetClientRect(window, &rect);

        int xPos = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
        int yPos = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;

        SetWindowPos(window, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        //clip cursor to inside of window area
        POINT ul;
        ul.x = rect.left;
        ul.y = rect.top;

        POINT lr;
        lr.x = rect.right;
        lr.y = rect.bottom;

        MapWindowPoints(window, HWND_DESKTOP, &ul, 1);
        MapWindowPoints(window, HWND_DESKTOP, &lr, 1);

        rect.left = ul.x;
        rect.top = ul.y;

        rect.right = lr.x;
        rect.bottom = lr.y;

        center.x = rect.left + (rect.right - rect.left) * 0.5f;
        center.y = rect.top + (rect.bottom - rect.top) * 0.5f;

        ClipCursor(&rect);

        //TODO: figure out how to hide cursor
        //or: alternatively, make it transparent
        //EDIT: due to the way olcConsoleGameEngine acquires a console this cannot be done,
        // since we are not creating the window ourselves we do not have control over it's internal
        // state like the cursor.

        map += L"################";
        map += L"#..............#";
        map += L"#.##.#.#.#.#.#.#";
        map += L"#.#............#";
        map += L"#..............#";
        map += L"#..............#";
        map += L"#......#.#......";
        map += L"#...............";
        map += L"#......#.#......";
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

        objects = {
            {5, 5, 0.0f, 0.0f, false, spriteLamp},
            {5, 6, 0.0f, 0.0f, false, spriteLamp},
            {4.5, 7.5f, 0.0f, 0.0f, false, spriteLamp},
        };

        return true;
    }

    virtual bool OnUserUpdate(float fElapsedTime)
    {
        //player input
        if (m_keys[L'I'].bHeld)
        {
            playerA -= 1.0f * fElapsedTime;
        }
        if (m_keys[L'P'].bHeld)
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
        if (m_keys[L'D'].bHeld)
        {
            playerX += cosf(playerA) * walkSpeed * fElapsedTime; //shouldn't these be the other way around?
            playerY -= sinf(playerA) * walkSpeed * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX -= cosf(playerA) * walkSpeed * fElapsedTime;
                playerY += sinf(playerA) * walkSpeed * fElapsedTime;
            }
        }

        if (m_keys[L'A'].bHeld)
        {
            playerX -= cosf(playerA) * walkSpeed * fElapsedTime;
            playerY += sinf(playerA) * walkSpeed * fElapsedTime;

            if (map[(int)playerY * mapWidth + (int)playerX] == '#')
            {
                playerX += cosf(playerA) * walkSpeed * fElapsedTime;
                playerY -= sinf(playerA) * walkSpeed * fElapsedTime;
            }
        }

        //exit
        if (m_keys[VK_ESCAPE].bPressed)
        {
            running = false;
        }

        //mouse input
        POINT cursor;
        GetCursorPos(&cursor);

        int delta = cursor.x - center.x;
        playerA += (float)delta * 0.0025f;

        SetCursorPos(center.x, center.y);
        
        //...

        //fire bullets
        //if (m_keys[VK_SPACE].bReleased)
        //{
        //    Object o;
        //    o.x = playerX;
        //    o.y = playerY;

        //    float noise = (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.1f;
        //    o.velocityX = sinf(playerA + noise) * 8.0f;
        //    o.velocityY = cosf(playerA + noise) * 8.0f;

        //    o.sprite = spriteFireBall;
        //    o.remove = false;
        //    listObjects.push_back(o);
        //}
        //...

        //rendering
        float dirX = std::sinf(playerA);
        float dirY = std::cosf(playerA);

        float perpX = std::sinf(playerA + (3.14159f * 0.5f));
        float perpY = std::cosf(playerA + (3.14159f * 0.5f));
    
        float posZ = 0.5f * ScreenHeight();

        //render the floor and ceiling
        for (int y = 0; y < ScreenHeight(); y++)
        {
            float vector1X = dirX - perpX;
            float vector1Y = dirY - perpY;
            float vector2X = dirX + perpX;
            float vector2Y = dirY + perpY;

            int p = y - ScreenHeight() * 0.5f;

            float rowDistance = posZ / p;

            float floorStepX = rowDistance * (vector2X - vector1X) / (float)ScreenWidth();
            float floorStepY = rowDistance * (vector2Y - vector1Y) / (float)ScreenWidth();

            float floorX = playerX + rowDistance * vector1X;
            float floorY = playerY + rowDistance * vector1Y;

            for (int x = 0; x < ScreenWidth(); x++)
            {
                float tx = floorX - std::floor(floorX);
                float ty = floorY - std::floor(floorY);

                if (y > posZ)
                {
                    Draw(x, y, spriteWall->SampleGlyph(tx, ty), spriteWall->SampleColour(tx, ty) + 2);
                }
                else
                {
                    Draw(x, y, spriteWall->SampleGlyph(tx, ty), spriteWall->SampleColour(tx, ty) + 1);
                }

                floorX += floorStepX;
                floorY += floorStepY;
            }
        }

        //render the walls
        for (int x = 0; x < ScreenWidth(); x++)
        {
            float cameraX = 2 * x / (float)ScreenWidth() - 1;

            //raycast vector
            float vectorX = dirX + perpX * cameraX;
            float vectorY = dirY + perpY * cameraX;

            //current map tile
            int mapX = (int)playerX;
            int mapY = (int)playerY;

            //length of raycast from current position to next x or y side
            float sideDistX;
            float sideDistY;

            //TODO: explanation
            float deltaDistX = (vectorX == 0) ? 1e30 : std::abs(1 / vectorX);
            float deltaDistY = (vectorY == 0) ? 1e30 : std::abs(1 / vectorY);

            int stepX;
            int stepY;

            bool hitWall = false;
            int side;

            if (vectorX < 0)
            {
                stepX = -1;
                sideDistX = (playerX - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1 - playerX) * deltaDistX;
            }
            if (vectorY < 0)
            {
                stepY = -1;
                sideDistY = (playerY - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1 - playerY) * deltaDistY;
            }

            //"march" forward from ray origin untill we hit a wall
            while (!hitWall)
            {
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }

                if (map[mapY * mapWidth + mapX] == '#')
                {
                    hitWall = true;
                }
            }

            float perpWallDist;
            if (side == 0)
            {
                perpWallDist = (sideDistX - deltaDistX);
            }
            else
            {
                perpWallDist = (sideDistY - deltaDistY);
            }

            //calculate height of line
            int lineheight = (int)(ScreenHeight() / perpWallDist);

            //calculate lowest and highest pixel to fill in current stripe
            int ceiling = -lineheight * 0.5f + ScreenHeight() * 0.5f;
            if (ceiling < 0) ceiling = 0;

            int floor = lineheight * 0.5f + ScreenHeight() * 0.5f;
            if (floor >= ScreenHeight()) floor = ScreenHeight() - 1;

            float wallX; //sample coordinate for texture, normalized
            if (side == 0) //use y-coordinate
            {
                wallX = playerY + perpWallDist * vectorY;
            }
            else //use x-coordinate
            {
                wallX = playerX + perpWallDist * vectorX;
            }

            wallX -= std::floor(wallX);

            float texX = wallX;
            if (side == 0 && vectorX > 0)
            {
                texX = 1 - texX;
            }
            if (side == 1 && vectorY < 0)
            {
                texX = 1 - texX;
            }

            //update depth buffer
            depthBuffer[x] = perpWallDist;

            float step = 1.0f / lineheight;
            float texPos = (ceiling - ScreenHeight() * 0.5f + lineheight * 0.5f) * step;
            //BUGBUG: without 0.03 offset this clips off the top part of the texture
            texPos += 0.03f;
            for (int y = ceiling; y <= floor; y++)
            {
                if (perpWallDist < maxDepth)
                {
                    float texY = texPos;
                    Draw(x, y, spriteWall->SampleGlyph(texX, texY), spriteWall->SampleColour(texX, texY));
                    texPos += step;
                }
                else //primarily doing this to prevent any weird rendering due to rounding errors from far distances
                {
                    Draw(x, y, PIXEL_SOLID, FG_BLACK);
                }
            }
        }
        //...

        //Update & Draw Objects
        std::vector<int> objectOrder;
        std::vector<float> objectDistance;

        int i = 0;
        for (int i = 0; i < objects.size(); i++)
        {
            objectOrder.push_back(i);
            objectDistance.push_back((playerX - objects[i].x)* (playerX - objects[i].x) + (playerY - objects[i].y) * (playerY - objects[i].y));
        }

        //order objectorder
        sortSprites(objectOrder, objectDistance, objects.size());

        for (int i = 0; i < objects.size(); i++)
        {
            olcSprite *sprite = objects[objectOrder[i]].sprite;

            float objectX = objects[objectOrder[i]].x - playerX;
            float objectY = objects[objectOrder[i]].y - playerY;

            //inverse determinant is 1 / determinant of matrix
            float invDeterminant = 1 / (perpX * dirY - dirX * perpY);

            //this is the multiplication between the object vector (objectX/objectY) and the inverse matrix for the camera plane:
            //determinant * matrix * vector <- in that order, remember is important for matrix multiplication
            float transformX = invDeterminant * (dirY * objectX - dirX * objectY);
            float transformY = invDeterminant * (-perpY * objectX + perpX * objectY); //acts as the z-depth inside the screen?

            int spriteScreenX = int((ScreenWidth() / 2) * (1 + transformX / transformY));

            //calculate sprite height on screen
            int spriteHeight = std::abs(int(ScreenHeight() / transformY)); //we use transformY here instead of the real distance to prevent the fisheye effect
            int drawStartY = -spriteHeight / 2 + ScreenHeight() / 2;
            if (drawStartY < 0) drawStartY = 0;
            int drawEndY = spriteHeight / 2 + ScreenHeight() / 2;
            if (drawEndY >= ScreenHeight()) drawEndY = ScreenHeight() - 1;

            float objectAspectRatio = (float)sprite->nWidth / (float)sprite->nHeight;

            //calculate sprite width on screen
            //int spriteWidth = std::abs(int(ScreenWidth() / transformY)); //why Y here?
            int spriteWidth = spriteHeight * objectAspectRatio;
            int drawStartX = -spriteWidth / 2 + spriteScreenX;
            if (drawStartX < 0) drawStartX = 0;
            int drawEndX = spriteWidth / 2 + spriteScreenX;
            if (drawEndX >= ScreenWidth()) drawEndX = ScreenWidth() - 1;

            for (int stripe = drawStartX; stripe < drawEndX; stripe++)
            {
                float texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * sprite->nWidth / spriteWidth) / 256;

                if (transformY > 0 && stripe > 0 && stripe < ScreenWidth() && transformY < depthBuffer[stripe])
                {
                    for (int y = drawStartY; y < drawEndY; y++)
                    {
                        int d = y * 256 - ScreenHeight() * 128 + spriteHeight * 128; //256 and 128 factors to avoid floats
                        float texY = ((d * sprite->nHeight) / spriteHeight) / 256;
                        if (sprite->SampleColour(texX / sprite->nWidth, texY / sprite->nHeight) != FG_BLACK)
                        {
                            Draw(stripe, y, sprite->SampleGlyph(texX / sprite->nWidth, texY / sprite->nHeight), sprite->SampleColour(texX / sprite->nWidth, texY / sprite->nHeight));
                        }
                    }
                }
            }
        }

        //TODO

        //for (auto& object : listObjects)
        //{
        //    //update object physics
        //    object.x += object.velocityX * fElapsedTime;
        //    object.y += object.velocityY * fElapsedTime;

        //    //check if object is inside wall, if so, set flag for removal
        //    if (map.c_str()[(int)object.x * mapWidth + (int)object.y] == '#')
        //    {
        //        object.remove = true;
        //    }

        //    //is object within distance?
        //    float vecX = object.x - playerX;
        //    float vecY = object.y - playerY;
        //    float distanceFromPlayer = sqrt(vecX * vecX + vecY * vecY);

        //    float eyeX = sinf(playerA);
        //    float eyeY = cosf(playerA);
        //    float objectAngle = atan2f(eyeY, eyeX) - atan2f(vecY, vecX);
        //    if (objectAngle < -3.14159f) //make sure angle lies between 0 and +-2*pi?
        //    {
        //        objectAngle += 2.0f * 3.14159f;
        //    }
        //    if (objectAngle > 3.14159f)
        //    {
        //        objectAngle -= 2.0f * 3.14159f;
        //    }

        //    bool inPlayerFOV = fabs(objectAngle) < FOV / 2.0f;

        //    if (inPlayerFOV && distanceFromPlayer >= 0.5f && distanceFromPlayer < maxDepth)
        //    {
        //        float objectCeiling = (float)(ScreenHeight() / 2.0) - ScreenHeight() / ((float)distanceFromPlayer);
        //        float objectFloor = ScreenHeight() - objectCeiling;
        //        float objectHeight = objectFloor - objectCeiling;
        //        float objectAspectRatio = (float)object.sprite->nHeight / (float)object.sprite->nWidth;
        //        float objectWidth = objectHeight / objectAspectRatio;

        //        float middleOfObject = (0.5f * (objectAngle / (FOV / 2.0f)) + 0.5f) * (float)ScreenWidth(); //?

        //        for (float x = 0; x < objectWidth; x++)
        //        {
        //            for (float y = 0; y < objectHeight; y++)
        //            {
        //                float sampleX = x / objectWidth;
        //                float sampleY = y / objectHeight;

        //                wchar_t c = object.sprite->SampleGlyph(sampleX, sampleY);
        //                int objectColumn = (int)(middleOfObject + x - (objectWidth / 2.0f));
        //                if (objectColumn >= 0 && objectColumn < ScreenWidth())
        //                {
        //                    if (c != L' ' && depthBuffer[objectColumn] >= distanceFromPlayer)
        //                    {
        //                        Draw(objectColumn, objectCeiling + y, c, object.sprite->SampleColour(sampleX, sampleY));
        //                        depthBuffer[objectColumn] = distanceFromPlayer;
        //                    }
        //                }
        //            }
        //        }
        //    }
        //}
        //...

        //remove dead objects from object list
        //listObjects.remove_if([](Object& o) {return o.remove; });

        //display map
        //for (int mx = 0; mx < mapWidth; mx++)
        //{
        //    for (int my = 0; my < mapHeight; my++)
        //    {
        //        Draw(mx + 1, my + 1, map[(mapHeight - my - 1) * mapWidth + mx]);
        //    }
        //}
        //Draw(1 + (int)playerX, 1 + (int)playerY, L'P');
        //...

        //stats
        //swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", playerX, playerY, playerA, 1.0f / deltaTime);
        //...

        return running;
    }

    virtual bool OnUserDestroy()
    {
        ClipCursor(NULL);

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
    game.ConstructConsole(320, 200, 4, 4);
    game.Start();

    return 0;
}