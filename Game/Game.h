#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        auto start = chrono::steady_clock::now(); //сохраняет текущее время на момент запуска игры
        if (is_replay) //если был нажат повтор партии
        {
            logic = Logic(&board, &config); //по параметрам конфига настраивается поведение бота
            config.reload(); //перезагрузка конфига
            board.redraw(); //приведение доски к стартовым значениям
        }
        else
        {
            board.start_draw(); //загружает спрайты и игровую доску
        }
        is_replay = false;

        int turn_num = -1; //текущий ход
        bool is_quit = false; //нужно ли закончить игру
        const int Max_turns = config("Game", "MaxNumTurns"); //загружает из конфига значения количества максимальных ходов в партии
        while (++turn_num < Max_turns)
        {
            beat_series = 0; //серия ходов шашек
            logic.find_turns(turn_num % 2); //определяет возможные ходы
            if (logic.turns.empty())
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel")); //определяет максимальную глубину просчетов ходов, опираясь на значение из конфига
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot"))) //определяет совершает ли текущий ход бот
            {
                auto resp = player_turn(turn_num % 2); //получает ход игрока
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break; //при нажатии кнопки выхода приложение закрывается
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break; //при нажатии кнопки игра начинается заново
                }
                else if (resp == Response::BACK) //при нажатии кпоки назад возвращет ходы
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&  //определяет чей был предыдущий ход
                        !beat_series && board.history_mtx.size() > 2) 
                    {
                        board.rollback(); //вернуть доску к предыдущему состоянию
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback(); //вернуть доску к предыдущему состоянию, возвращая ход текущему игроку
                    --turn_num;
                    beat_series = 0; 
                }
            }
            else
                bot_turn(turn_num % 2); //расчитывается ход бота
        }
        auto end = chrono::steady_clock::now(); //сохраняет время на момент конца хода
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close(); //логирует события совершенные за ход

        if (is_replay)
            return play(); //если реплей, то возвращаемся на начало хода
        if (is_quit)
            return 0; //выход из игры
        int res = 2; //результат если игру выйгралм черные
        if (turn_num == Max_turns) 
        {
            res = 0; //результат, если игра закончилась ничьей
        }
        else if (turn_num % 2)
        {
            res = 1; //результат, если игру выйграли белые
        }
        board.show_final(res); //вывести победителя
        auto resp = hand.wait(); //считать устройство ввода
        if (resp == Response::REPLAY) //нужно ли начать игру заново
        {
            is_replay = true;
            return play();
        }
        return res; 
    }

  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS");
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color);
        th.join();
        bool is_first = true;
        // making moves
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series);
        }

        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells; //создает массив для возможных ходов
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y); //сохраняет данные под возможные ходы
        }
        board.highlight_cells(cells); //подсвечивает возможные ходы
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1; //сбрасывает переменные в их начальное значение
        // trying to make first move
        while (true)
        {
            auto resp = hand.get_cell(); //ожидание ивента от игрока или выбраной им клетки
            if (get<0>(resp) != Response::CELL) //выход из цикла, если событие не выбор клетки
                return get<0>(resp); 
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; //создает переменную с выбраной игроком клетки

            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second) //на выбраной клетке есть фигура игрока
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second}) //клетка с возможным ходом
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break; //если не дефолтная позиция выйти из цикла
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells); //если фигура не выбрана, посветить допустимые ходы
                }
                x = -1;
                y = -1;
                continue; //если фигура не выбрана, начать цикл сначала
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y); //подсветить выбраную фигуру
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2); //собрать массив возможных ходов выбраной фигуры
                }
            }
            board.highlight_cells(cells2); //подсветить этот массив
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1); //если игрок подтвердил ход, очистить подсветы и передвинуть фигуру
        if (pos.xb == -1)
            return Response::OK;
        // continue beating while can
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats) //была ли съедена фигура за ход
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells); //подсвечивает все ходы для серии ходов
            board.set_active(pos.x2, pos.y2); // подсвечивает выбраную фигуру
            // trying to make move
            while (true)
            {
                auto resp = hand.get_cell(); //на какую ячейку кликнул игрок или какие действия он совершил
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct) 
                    continue; //повторение кода выше 

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series); //серия увеличивается на 1 и переставляет фигуру по итогам серии ходов
                break;
            }
        }

        return Response::OK;
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};

