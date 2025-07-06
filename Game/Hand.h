#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
  public:
    Hand(Board *board) : board(board)
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const //функция возращения тип событий действий игрока и выбраную клетку
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) //функция получения совершенного ивента
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT: 
                    resp = Response::QUIT; //ивент совершения выхода из приложения
                    break;
                case SDL_MOUSEBUTTONDOWN: //была нажата левая кнопка мыши
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; //была нажата иконка назад
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; //была нажата кнопка переигровки
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; //была выбрана клетка
                    }
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: //был изменен размер окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                if (resp != Response::OK) //если не состояние ОК, то возращает ивент который был до этого
                    break;
            }
        }
        return {resp, xc, yc};
    }

    Response wait() const //ожидание инпута от игрока
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) //функция получения совершенного ивента
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; //ивент совершения выхода из приложения
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: //размер окна приложения был изменен 
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: { //была нажата левая кнопка мыши
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; //была нажата кнопка переигровки
                }
                break;
                }
                if (resp != Response::OK) //если не состояние ОК, то продолжает ожидание инпута от игрока
                    break; 
            }
        }
        return resp;
    }

  private:
    Board *board;
};
