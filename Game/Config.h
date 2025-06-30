#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }
    
    void reload() //метод который открывает заново файл настройки и считывает в переменную
    {
        std::ifstream fin(project_path + "settings.json"); //открывает файл по пути project_path с названием settings.json
        fin >> config; //считывает данные в переменныю config
        fin.close(); //закрывает считывание файла
    }

    auto operator()(const string &setting_dir, const string &setting_name) const //для более удобного доступа к данным, полученные из файла настройки
    {
        return config[setting_dir][setting_name]; //возвращает значение json по ключу
    }

  private:
    json config;
};
