#pragma once
#include <iostream>
#include <fstream>
#include <vector>

#include "../Models/Move.h"
#include "../Models/Project_path.h"

#ifdef __APPLE__
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_image.h>
#else
    #include <SDL.h>
    #include <SDL_image.h>
#endif

using namespace std;

class Board
{
public:
    Board() = default;
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // draws start board
    int start_draw() //инциализция библиотеки sdl, загрузка ресурсов и отрисовка начального состояния доски
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) //инициализация модулей sdl
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }
        if (W == 0 || H == 0)
        {
            SDL_DisplayMode dm; //получение разрешения экрана
            if (SDL_GetDesktopDisplayMode(0, &dm))
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            W = min(dm.w, dm.h);
            W -= W / 15;
            H = W;
        }
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE); //создание графического окна
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); //создание контекста для отрисовки на экран
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }
        board = IMG_LoadTexture(ren, board_path.c_str()); //загрузка текстур элементов игры
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay) //проверка корректности загрузки ресурсов игры
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }
        SDL_GetRendererOutputSize(ren, &W, &H);
        make_start_mtx(); //создание матрицы для перебора ходов
        rerender(); //отрисовка
        return 0;
    }

    void redraw() //функция перерисовки и приведения к начальным параметрам
    {
        game_results = -1;
        history_mtx.clear();
        history_beat_series.clear();
        make_start_mtx();
        clear_active();
        clear_highlight();
    }

    void move_piece(move_pos turn, const int beat_series = 0) //функция изменения положения фигуры
    {
        if (turn.xb != -1)
        {
            mtx[turn.xb][turn.yb] = 0;
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series);
    }

    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0) //функция изменения положения фигуры
    {
        if (mtx[i2][j2])
        {
            throw runtime_error("final position is not empty, can't move");
        }
        if (!mtx[i][j])
        {
            throw runtime_error("begin position is empty, can't move");
        }
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7))
            mtx[i][j] += 2;
        mtx[i2][j2] = mtx[i][j];
        drop_piece(i, j);  //убирает фигуру с доски
        add_history(beat_series); //добавляет историю съеденных фигур
    }

    void drop_piece(const POS_T i, const POS_T j) //удаляет фигуру и перерисовывает доску
    {
        mtx[i][j] = 0;
        rerender();
    }

    void turn_into_queen(const POS_T i, const POS_T j) //меняет шашку на дамку и перерисовывает доску
    {
        if (mtx[i][j] == 0 || mtx[i][j] > 2)
        {
            throw runtime_error("can't turn into queen in this position");
        }
        mtx[i][j] += 2;
        rerender();
    }
    vector<vector<POS_T>> get_board() const
    {
        return mtx;
    }

    void highlight_cells(vector<pair<POS_T, POS_T>> cells) //подсвечивает данные клетки
    {
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1;
        }
        rerender();
    }

    void clear_highlight() //убрать подсвеченные клетки
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0);
        }
        rerender();
    }

    void set_active(const POS_T x, const POS_T y) //установить данную клетку как активную
    {
        active_x = x;
        active_y = y;
        rerender();
    }

    void clear_active() //очистить установленную активную клетку
    {
        active_x = -1;
        active_y = -1;
        rerender();
    }

    bool is_highlighted(const POS_T x, const POS_T y) //проверка: подсвечена ли клетка
    {
        return is_highlighted_[x][y];
    }

    void rollback() //переход к предыдущему ходу
    {
        auto beat_series = max(1, *(history_beat_series.rbegin())); //получение истории съеденных фигур
        while (beat_series-- && history_mtx.size() > 1)
        {
            history_mtx.pop_back();
            history_beat_series.pop_back();
        }
        mtx = *(history_mtx.rbegin());
        clear_highlight();
        clear_active();
    }

    void show_final(const int res) //вывести результат игры
    {
        game_results = res;
        rerender();
    }

    // use if window size changed
    void reset_window_size() //обновить размер окна приложения
    {
        SDL_GetRendererOutputSize(ren, &W, &H);
        rerender();
    }

    void quit() //очистить ресурсы по завершению работы приложения
    {
        SDL_DestroyTexture(board);
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

    ~Board()
    {
        if (win)
            quit();
    }

private:
    void add_history(const int beat_series = 0) //добавляет в историю съеденные фигуры
    {
        history_mtx.push_back(mtx);
        history_beat_series.push_back(beat_series);
    }
    // function to make start matrix
    void make_start_mtx() //создает матрицу возможных ходов
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0;
                if (i < 3 && (i + j) % 2 == 1)
                    mtx[i][j] = 2;
                if (i > 4 && (i + j) % 2 == 1)
                    mtx[i][j] = 1;
            }
        }
        add_history();
    }

    // function that re-draw all the textures
    void rerender() //отрисока всех элементов игры
    {
        // draw board
        SDL_RenderClear(ren); //очистка экрана
        SDL_RenderCopy(ren, board, NULL, NULL); 

        // draw pieces
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j])
                    continue;
                int wpos = W * (j + 1) / 10 + W / 120;
                int hpos = H * (i + 1) / 10 + H / 120;
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 };

                SDL_Texture* piece_texture;
                if (mtx[i][j] == 1)
                    piece_texture = w_piece;
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece;
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen;
                else
                    piece_texture = b_queen;

                SDL_RenderCopy(ren, piece_texture, NULL, &rect); //отрисовка фигур
            }
        }

        // draw hilight
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0); //изменения цвета отрисовки
        const double scale = 2.5;
        SDL_RenderSetScale(ren, scale, scale); //изменение масштаба отрисовки
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j])
                    continue;
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell); //отрисовка подсветки с помощью квадрата
            }
        }

        // draw active
        if (active_x != -1)
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0); //изменение цвета отрисовки
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell); //отрисовка подсветки для активных фигур с помощью квадрата 
        }
        SDL_RenderSetScale(ren, 1, 1); //установить масштаб отрисовки

        // draw arrows
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 }; 
        SDL_RenderCopy(ren, back, NULL, &rect_left); //отрисовать текстуры для кнопки перехода назад
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, replay, NULL, &replay_rect); //отрисовать текстуры для кнопки перезагрузки

        // draw result(отрисовка итого игры)
        if (game_results != -1)
        {
            string result_path = draw_path;
            if (game_results == 1) //выбор текстуры по итогам игры
                result_path = white_path;
            else if (game_results == 2)
                result_path = black_path;
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());  //загрузка нужной текстуры
            if (result_texture == nullptr)
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path); // вывод ошибки при провальной загрузки текстуры
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect); //копирование текстуры в рендер
            SDL_DestroyTexture(result_texture); //удаление текстуры
        }

        SDL_RenderPresent(ren); //вывод кадра на экран
        // next rows for mac os
        SDL_Delay(10); //задержка после вывода кадра 10 мс
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent); //получить ивенты от пользователя
    }

    void print_exception(const string& text) { //вывод исключений
        ofstream fout(project_path + "log.txt", ios_base::app); //открыть текстовый файл
        fout << "Error: " << text << ". "<< SDL_GetError() << endl; //выводит ошибку в текстовый файл
        fout.close(); //закрывает файл
    }

  public:
    int W = 0;
    int H = 0;
    // history of boards(история ходов)
    vector<vector<vector<POS_T>>> history_mtx;

  private:
    SDL_Window *win = nullptr;
    SDL_Renderer *ren = nullptr;
    // textures(указатели на текстуры)
    SDL_Texture *board = nullptr;
    SDL_Texture *w_piece = nullptr;
    SDL_Texture *b_piece = nullptr;
    SDL_Texture *w_queen = nullptr;
    SDL_Texture *b_queen = nullptr;
    SDL_Texture *back = nullptr;
    SDL_Texture *replay = nullptr;
    // texture files names(пути до текстур)
    const string textures_path = project_path + "Textures/";
    const string board_path = textures_path + "board.png";
    const string piece_white_path = textures_path + "piece_white.png";
    const string piece_black_path = textures_path + "piece_black.png";
    const string queen_white_path = textures_path + "queen_white.png";
    const string queen_black_path = textures_path + "queen_black.png";
    const string white_path = textures_path + "white_wins.png";
    const string black_path = textures_path + "black_wins.png";
    const string draw_path = textures_path + "draw.png";
    const string back_path = textures_path + "back.png";
    const string replay_path = textures_path + "replay.png";
    // coordinates of chosen cell(координаты активной ячейки)
    int active_x = -1, active_y = -1;
    // game result if exist(результат окончания игры)
    int game_results = -1;
    // matrix of possible moves(массив с подсвечеными клетками)
    vector<vector<bool>> is_highlighted_ = vector<vector<bool>>(8, vector<bool>(8, 0));
    // matrix of possible moves(матрица возможных ходов)
    // 1 - white, 2 - black, 3 - white queen, 4 - black queen(идентификаторы текстур фигур)
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));
    // series of beats for each move(история возможных ходов)
    vector<int> history_beat_series; //история ходов в серии
};
