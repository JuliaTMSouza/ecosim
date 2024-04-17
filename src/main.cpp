#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <mutex>
#include <condition_variable>

static const uint32_t NUM_ROWS = 15;

std::mutex grid;
std::condition_variable cv;
bool wake;
std::mutex mtx_cv;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore,
    morta
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

bool getProbability(double prob)
{
    double probPercentage = prob * 100;
    int random_number = rand() % 100;
    std::cout << random_number << std::endl;
    return random_number < probPercentage;
}

void simulatePlant(int i, int j)
{
    std::unique_lock<std::mutex> lock(mtx_cv);
    while (true)
    {
        cv.wait(lock);
        grid.lock();
        if (entity_grid[i][j].type == morta || entity_grid[i][j].age == PLANT_MAXIMUM_AGE)
        {
            entity_grid[i][j].type = empty;
            entity_grid[i][j].age = 0;
            break;
        }

        if (i + 1 < NUM_ROWS)
        {
            std::cout << "i + 1" << (entity_grid[i + 1][j].type == empty) << std::endl;
            if (entity_grid[i + 1][j].type == empty && getProbability(PLANT_REPRODUCTION_PROBABILITY))
            {
                std::cout << "i " << j + 1 << " j " << j << std::endl;
                entity_grid[i + 1][j].type = plant;
            }
        }
        if (j + 1 < NUM_ROWS)
        std::cout << "j+1" << (entity_grid[i][j + 1].type == empty) << std::endl;
        {
            if (entity_grid[i][j + 1].type == empty && getProbability(PLANT_REPRODUCTION_PROBABILITY))
            {
                std::cout << "i " << i << " j " << j + 1 << std::endl;
                entity_grid[i][j + 1].type = plant;
            }
        }
        if (j - 1 > 0)
        {
            std::cout << "j-1" << (entity_grid[i][j - 1].type == empty) << std::endl;
            if (entity_grid[i][j - 1].type == empty && getProbability(PLANT_REPRODUCTION_PROBABILITY))
            {
                std::cout << "i " << i << " j " << j - 1 << std::endl;
                entity_grid[i][j - 1].type = plant;
            }
        }
        if (i - 1 > 0)
        {
            std::cout << "i-1" << (entity_grid[i - 1][j].type == empty) << std::endl;
            if (entity_grid[i - 1][j].type == empty && getProbability(PLANT_REPRODUCTION_PROBABILITY))
            {
                std::cout << "i " << i - 1 << " j " << j << std::endl;
                entity_grid[i - 1][j].type = plant;
            }
        }
        entity_grid[i][j].age++;
        grid.unlock();
    }
}

int randomNumber()
{
    int number = rand() % NUM_ROWS;
    return number;
}

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities
        // <YOUR CODE HERE>

        for (int i = 0; i < (uint32_t)request_body["plants"]; i++) {
            entity_t newPlant;
            newPlant.age = 0;
            newPlant.type = plant;
            int foundPos = 0;
            int row;
            int col;
            std::cout << i << std::endl;
            while (foundPos == 0){
                row = randomNumber();
                col = randomNumber();
                std::cout << entity_grid[row][col].type << std::endl;
                if(entity_grid[row][col].type == empty) {
                    entity_grid[row][col] = newPlant;
                    foundPos = 1;
                }
            }
        }

        for (int i = 0; i < (uint32_t)request_body["carnivores"]; i++) {
            entity_t newCarnivore;
            newCarnivore.age = 0;
            newCarnivore.type = carnivore;
            newCarnivore.energy = 0;
            int foundPos = 0;
            int row;
            int col;
            while (foundPos == 0){
                row = randomNumber();
                col = randomNumber();
                if(entity_grid[row][col].type == empty) {
                    entity_grid[row][col] = newCarnivore;
                    foundPos = 1;
                }
            }
        }

        for (int i = 0; i < (uint32_t)request_body["herbivores"]; i++) {
            entity_t newHerbivore;
            newHerbivore.age = 0;
            newHerbivore.type = herbivore;
            newHerbivore.energy = 0;
            int foundPos = 0;
            int row;
            int col;
            while (foundPos == 0){
                row = randomNumber();
                col = randomNumber();
                if(entity_grid[row][col].type == empty) {
                    entity_grid[row][col] = newHerbivore;
                    foundPos = 1;
                }
            }
        }
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        
        for (int i = 0; i < NUM_ROWS; i++) {
            for (int j = 0; j < NUM_ROWS; j++) {
                if (i == NUM_ROWS - 1 && j == NUM_ROWS - 1) {
                    std::cout << "mudou wake" << std::endl;

                    
        cv.notify_all();
                } else {
                    std::unique_lock<std::mutex> lock(mtx_cv);
                    wake = 0;
                }
                if (entity_grid[i][j].type == plant && entity_grid[i][j].age == 0) {
                    std::cout << "Hello" << std::endl;
                    std::thread p(simulatePlant,i,j); 
                    p.detach();
                }
            }
        }
      
        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}