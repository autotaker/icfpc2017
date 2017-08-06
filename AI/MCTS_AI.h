#pragma once

#include "../lib/Game.h"

class MCTS_AI : public Game {
	SetupSettings setup() const override;
	tuple<int, int, Json::Value> move() const override;
	
	std::string name() const override;
};
