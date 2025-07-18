#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

struct Position {
    POS_T x = -1;
    POS_T y = -1;

    bool is_initized() {
        return (x != -1) && (y != -1);
    }
};

class Logic
{
public:
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color)
    {
        clear_search_data();  // Clear previous search data
        perform_initial_search(color);  // Start the recursive search
        return extract_best_move_sequence();  // Return the found sequence
    }

private:

    void clear_search_data()
    {
        next_best_state.clear();
        next_move.clear();
    }

    void perform_initial_search(const bool color)
    {
        find_first_best_turn(board->get_board(), color, { -1, -1 }, 0);
    }

    vector<move_pos> extract_best_move_sequence()
    {
        int cur_state = 0;
        vector<move_pos> res;
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (!is_end_of_sequence(cur_state));
        return res;
    }

    bool is_end_of_sequence(int state) const
    {
        return state == -1 || next_move[state].x == -1;
    }

    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) //���� ����� ����
            mtx[turn.xb][turn.yb] = 0; //��������� �������� ������ ������ �� ����������� �����
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))//���� ����� ��� ������ ����� ������� �� ���������� ���� �����
            mtx[turn.x][turn.y] += 2; //����� ���������� ���������
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; //�������� ��� ������ � ����� ������
        mtx[turn.x][turn.y] = 0; //������ ������ ������ ����������� �����
        return mtx;
    }

    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const //���� ������ ���� (��������� ����)
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0; //���������� ����������� ����� �� ���� ������� ����
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                //������� ���� ����� � ����
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential") //������������ ����������� ������ �� ����� ��� ������� ���, �������� �� ����������� ����� � ��������� �������� �����
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color) //����������� ������������ ����, ���� ���� ���� �����
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF; //���������� �������������(��������������� ������) ���� �� ����� ���� ������ ��� ������ ����������
        if (b + bq == 0)
            return 0; //���������� 0(������ ���) ���� �� ����� ����� ������ ��� ���� ������
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential") //������� ����������� ������� ����������� ������� �� ����
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef); //���������� ��������� ������������ ������ ������ � �����
    }

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const Position& pos, size_t state, double alpha = -1)
    {
        state_clear();

        // ���� ��� �� ��������� ���������, ���� ��������� ���� ��� ������� �������
        if (state != 0)
            find_turns(pos.x, pos.y, mtx);

        // ���� ��� ������ � ��� �� ��������� ���������, ��������� � ������������ ������
        if (is_initial_state(have_beats, state))
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }


        return find_best_score(mtx, color, state, turns, have_beats);
    }


    void state_clear() {
        // ������������� ������ ���������
        next_best_state.push_back(-1); // ��������� ��������� ��������� ���������
        next_move.emplace_back(-1, -1, -1, -1); // ��������� ��������� ���
    }

    bool is_initial_state(bool have_beats_now, size_t state) {
        if (!have_beats_now && state != 0) {
            return true;
        }
        return false;
    }

    double find_best_score(vector<vector<POS_T>> mtx, const bool color, size_t state, std::vector<move_pos> turns, bool have_beats) {
        double best_score = -1; // ������������� ������� �����

        // ������� ��� �������� ������ ����� � ���������
        vector<move_pos> best_moves;
        vector<int> best_states;

        // ���������� ��� ��������� ����
        for (auto turn : turns)
        {
            size_t next_state = next_move.size(); // �������� ������ ���������� ���������
            double score;

            // ���� ���� ������, ���������� ����� � �������
            if (have_beats)
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, { turn.x2, turn.y2 }, next_state, best_score);
            }
            else // ����� ��������� ������� ����������� �����
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }

            // ��������� ������ ���� � ��������� ������ ���
            if (score > best_score)
            {
                best_score = score;
                next_best_state[state] = (have_beats ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        return best_score; // ���������� ������ ���� ��� �������� ���������
    }


    struct SearchParams {
        bool color;
        size_t depth;
        double alpha;
        double beta;
    };

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1, double beta = INF + 1, Position pos = { -1, -1 })
    {
        // ������� ������ �������� - ���������� ������������ �������
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color));
        }

        // ���� ������ ����������, ���� ���� ��� ���������� �������
        if (pos.is_initized())
        {
            find_turns(pos.x, pos.y, mtx);
        }
        else // ����� ���� ��� ��������� ���� ��� �������� �����
        {
            find_turns(color, mtx);
        }

        // ���� ��� ������ � ��� ����������� ����� �����, ����������� ������
        if (!have_beats && pos.is_initized())
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // ���� ��� ��������� �����
        if (turns.empty())
            return (depth % 2 ? 0 : INF); // ���������� 0 ��� INF � ����������� �� �������

        SearchParams params = { color, depth, alpha, beta };
        return find_score(mtx, params, turns, have_beats, pos);
    }

    double find_score(vector<vector<POS_T>> mtx, SearchParams& params, std::vector<move_pos> turns, bool have_beats, Position pos) {
        // ������������� ������������ � ������������� ������
        double min_score = INF + 1;
        double max_score = -1;

        // ���������� ��� ��������� ����
        for (auto turn : turns)
        {
            double score = 0.0;
            // ���� ��� ������ � ��� ����� ��� (�� �����������)
            if (!have_beats && !pos.is_initized())
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - params.color, params.depth + 1, params.alpha, params.beta);
            }
            else // ����� ���������� ������� ����� �����
            {
                score = find_best_turns_rec(make_turn(mtx, turn), params.color, params.depth, params.alpha, params.beta, { turn.x2, turn.y2 });
            }

            // ��������� ����������� � ������������ �����
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // �����-���� ���������
            if (params.depth % 2)
                params.alpha = max(params.alpha, max_score);
            else
                params.beta = min(params.beta, min_score);

            // ���� ����������� ��������, ������ ��������������� �����
            if (optimization != "O0" && params.alpha >= params.beta)
                return (params.depth % 2 ? max_score + 1 : min_score - 1);
        }
        // ���������� ���� � ����������� �� ������� (��� ��� ����)
        return (params.depth % 2 ? max_score : min_score);
    }

public:
    void find_turns(const bool color) //���������� ������� ��� �������� ������������� � ���������� �� ���������
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y) //����� ������� ���� ��� ������
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns; //������ � ���������� ���������� ������
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx); //����� ��� ��� ����� � ������� ������������
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end()); //�������� ���� � ��������� ����
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng); //��������� ������� ��������� �����
        have_beats = have_beats_before;
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx) //������� ���������� ����� ��� ������ ��������� �����
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y]; //���������� ��� ����� 1 ����� ����� 2 ������ ����� 3 ����� ����� 5 ������ �����
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7) //����� ��������� � �������� ����
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2; //������� ���������� ����
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2) //���������, ��� ������ �� ������� �� ����� �� ������ � ������ � ������� ����� ������
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb); //���������� ��� ��� 1 �� ���������
                }
            }
            break;
        default:
            // check queens(����� ���� ��� �������� ���������� ��� �����)
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

public:
    vector<move_pos> turns; //��������� ��������� ���� 
    bool have_beats; //���� �� �� ��� ���� ������� �����
    int Max_depth; //������� �������� ���� ��� ����

private:
    default_random_engine rand_eng; //��������� ��������� �����
    string scoring_mode; //�������� ����������� ����� ���� ��� ����� ���� "�����"
    string optimization; //�������� �� ������������� ����� ��� ������� � ������� ��� ������ �������� ��������� ����
    vector<move_pos> next_move; //������ ��������� �������� �������� ��� ��� ����
    vector<int> next_best_state; //������ �������� ������� ���� ��� ���������� ����
    Board* board; //����� ���������� �� ������ �������� ���� � ��� ���������
    Config* config; //����� �������� � ����������� ������ �� settings.json 
};