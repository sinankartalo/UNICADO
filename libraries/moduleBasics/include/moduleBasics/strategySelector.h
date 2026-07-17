/*
 * UNICADO - UNIversity Conceptual Aircraft Design and Optimization
 *
 * Copyright (C) 2025 UNICADO consortium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * This file is part of UNICADO.
 */

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_STRATEGYSELECTOR_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_STRATEGYSELECTOR_H_

#include <moduleBasics/runtimeIO.h>
#include <memory>
#include <utility>
#include <stdexcept>

/**
 * \brief Base Strategy class with pure virtual implementation
 *
 */
class Strategy {
 public:
    /**
     * \brief Initialize your strategy (read your configuration information and data)
     *
     */
    virtual void initialize() = 0;

    /**
     * \brief Run your strategy (add your algorithms here)
     *
     */
    virtual void run() = 0;

    /**
     * \brief Update values in aircraft xml via your module data methods (update(...) or update...(...))
     *
     */
    virtual void update() = 0;

    /**
     * \brief Define and generate plots, define and generate reports
     *
     */
    virtual void report() = 0;

    /**
     * \brief Save module specific xml's if necessary (do not close acxml or moduleconfig node) from RuntimeIO object
     *
     */
    virtual void save() = 0;

    /**
     * \brief Destroy the Strategy object
     *
     */
    virtual ~Strategy() {}
};

using strategyptr = std::unique_ptr<Strategy>(const std::shared_ptr<RuntimeIO>&);
using strategyaccess = std::function<strategyptr>;

/**
 * \brief Strategy selector class for strategies class
 *
 */
class StrategySelector {
 public:
    /**
     * \brief Register a strategy object
     * \param S Strategy to associate with path
     * \param path Path to the associated strategy S as a vector of strings (ordered)
     */
    template<typename S>
    void registerStrategy(const std::vector<std::string>& path) {
        /* Check if path is available */
        Node* current = &root;
        try {
            if (path.empty()) {
                throw std::invalid_argument("Path is empty!");
            }
            /* Initialize current as root */

            /* Loop over all elements of the tree */
            for (const std::string& level : path) {
                if (current->children.find(level) == current->children.end()) {
                    current->children[level] = Node();
                }
                current = &current->children[level];
            }

            /* Check if strategy already registered */
            if (current->strategy != nullptr) {
                throw std::invalid_argument("Strategy already registered!");
            }
        }
        catch(std::exception& ex) {
            std::cerr << ex.what() << std::endl;
            exit(1);
        }
        /* Store a lambda that creates an instance of the wanted strategy which inherits from a strategy */
        current->strategy = [](const std::shared_ptr<RuntimeIO>& arg) -> std::unique_ptr<Strategy> {
            return std::make_unique<S>(arg);
        };
    }

    /**
     * \brief Set Strategy object
     *
     * \param path - vector of strings which defines the path to the wanted strategy
     * \param arg  - RuntimeIO shared pointer object reference
     */
    void setStrategy(const std::vector<std::string>& path, const std::shared_ptr<RuntimeIO>& arg) {

        const Node* current = &root;
        /* Loop over path */
        try {
            for (const std::string& level : path) {
                auto it = current->children.find(level);
                /* If strategy not in tree */
                if (it == current->children.end()) {
                    throw std::invalid_argument("Strategy does not exist. Abort!");
                }
                /* Set child to current */
                current = &it->second;
            }
            /* Path exists but no strategy is set */
            if (!current->strategy) {
                throw std::invalid_argument("No strategy registered for the specified route. Abort!");
            }
        }
        catch(std::exception& ex) {
            std::cerr << ex.what() << std::endl;
            exit(1);
        }
        /* Set strategy to selected by path with arg */
        strategy_ = std::move(current->strategy(arg));
    }

    /**
     * \brief Set the Strategy object
     *
     * \param strategy unique pointer of selected strategy
     */
    void setStrategy(std::unique_ptr<Strategy> strategy) {
        strategy_ = std::move(strategy);
    }

    /**
     * \brief Run initialize method of strategy
     *
     */
    void initializeStrategy() {
        strategy_->initialize();
    }

    /**
     * \brief Run run method of strategy
     *
     */
    void runStrategy() {
        strategy_->run();
    }

    /**
     * \brief Run update method of strategy
     *
     */
    void updateStrategy() {
        strategy_->update();
    }

    /**
     * \brief Run report method of strategy
     *
     */
    void reportStrategy() {
        strategy_->report();
    }

    /**
     * \brief Save report method of strategy
     *
     */
    void saveStrategy() {
        strategy_->save();
    }

 private:
    struct Node {
        std::map<std::string, Node> children;
        strategyaccess strategy;
    };
    std::unique_ptr<Strategy> strategy_; /* Selected strategy object by setStrategy */
    Node root;
};



#endif // MODULEBASICS_INCLUDE_MODULEBASICS_STRATEGYSELECTOR_H_
