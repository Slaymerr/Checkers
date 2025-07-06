#pragma once
#include <stdlib.h>

typedef int8_t POS_T; //позиция на поле

struct move_pos //структура, которая содержит информацию о ходе
{
    POS_T x, y;             // from(откуда)
    POS_T x2, y2;           // to(куда)
    POS_T xb = -1, yb = -1; // beaten(координаты съеденной фигуры)

    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2) //инициализатор ходов без координат съеденной фигуры
    {
    }
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb) //инициализатор ходов с съеденной фигурой
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    bool operator==(const move_pos &other) const //оператор сравнения ходов
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2); 
    }
    bool operator!=(const move_pos &other) const //оператор сравнения неравенства ходов
    {
        return !(*this == other);
    }
};
