#pragma once

#include "../lib/Game.h"

class MCTS_AI : public Game {
	SetupSettings setup() const override;
	MoveResult move() const override;
	
	std::string name() const override;
};
