#pragma once

#include "resource.h"
#include "script/effect/effect.h"

namespace stratagus {

class resource;

class resource_effect final : public effect
{
public:
	resource_effect(const resource *resource, const std::string &value, const sml_operator effect_operator)
		: effect(effect_operator), resource(resource)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "resource";
		return identifier;
	}

	virtual void do_assignment_effect(CPlayer *player) const override
	{
		player->set_resource(this->resource, this->quantity);
	}

	virtual void do_addition_effect(CPlayer *player) const override
	{
		player->change_resource(this->resource, this->quantity);
	}

	virtual void do_subtraction_effect(CPlayer *player) const override
	{
		player->change_resource(this->resource, -this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set " + this->resource->get_name() + " to " + std::to_string(this->quantity);
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + std::to_string(this->quantity) + " " + this->resource->get_name();
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + std::to_string(this->quantity) + " " + this->resource->get_name();
	}

private:
	const resource *resource = nullptr;
	int quantity = 0;
};

}
