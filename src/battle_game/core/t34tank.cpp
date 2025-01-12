#include "t34tank.h"

#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"

namespace battle_game::unit {

namespace {
uint32_t tank_body_model_index = 0xffffffffu;
uint32_t tank_turret_model_index = 0xffffffffu;
}  

T34Tank::T34Tank(GameCore *game_core, uint32_t id, uint32_t player_id)
    : Unit(game_core, id, player_id) {
  if (!~tank_body_model_index) {
    auto mgr = AssetsManager::GetInstance();

    tank_body_model_index = mgr->RegisterModel(
        {
            {{-0.8f, 0.8f}, {0.0f, 0.0f}, {0.5f, 1.0f, 0.5f, 1.0f}},
            {{-0.8f, -1.0f}, {0.0f, 0.0f}, {0.5f, 1.0f, 0.5f, 1.0f}},
            {{0.8f, 0.8f}, {0.0f, 0.0f}, {0.5f, 1.0f, 0.5f, 1.0f}},
            {{0.8f, -1.0f}, {0.0f, 0.0f}, {0.5f, 1.0f, 0.5f, 1.0f}},
        },
        {0, 1, 2, 1, 2, 3});

    std::vector<ObjectVertex> turret_vertices;
    std::vector<uint32_t> turret_indices;
    const int precision = 60;
    const float inv_precision = 1.0f / precision;
    for (int i = 0; i < precision; i++) {
      auto theta = (i + 0.5f) * inv_precision * glm::pi<float>() * 2.0f;
      turret_vertices.push_back({{std::sin(theta) * 0.5f, std::cos(theta) * 0.5f}, 
                                  {0.0f, 0.0f}, {0.7f, 0.7f, 1.0f, 1.0f}});
      turret_indices.push_back(i);
      turret_indices.push_back((i + 1) % precision);
      turret_indices.push_back(precision);
    }
    turret_vertices.push_back({{0.0f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 1.0f, 1.0f}});
    tank_turret_model_index = mgr->RegisterModel(turret_vertices, turret_indices);
  }
}

void T34Tank::Render() {
  battle_game::SetTransformation(position_, rotation_);
  battle_game::SetTexture(0);
  battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
  battle_game::DrawModel(tank_body_model_index);
  battle_game::SetRotation(turret_rotation_);
  battle_game::DrawModel(tank_turret_model_index);
}

void T34Tank::Update() {
  Move(3.0f, glm::radians(180.0f));
  RotateTurret();
  Fire();
  UseSpeedBoost();
}

void T34Tank::Move(float move_speed, float rotate_angular_speed) {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input = player->GetInputData();
    glm::vec2 offset{0.0f};
    if (input.key_down[GLFW_KEY_W]) offset.y += 1.0f;
    if (input.key_down[GLFW_KEY_S]) offset.y -= 1.0f;

    
    if (is_speed_boost_active_) {
      move_speed *= 2.0f;  
    }

    offset *= kSecondPerTick * move_speed;
    auto new_position = position_ + glm::vec2{glm::rotate(glm::mat4(1.0f), rotation_, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(offset, 0.0f, 0.0f)};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }

    float rotation_offset = 0.0f;
    if (input.key_down[GLFW_KEY_A]) rotation_offset += 1.0f;
    if (input.key_down[GLFW_KEY_D]) rotation_offset -= 1.0f;

    rotation_offset *= kSecondPerTick * rotate_angular_speed;
    game_core_->PushEventRotateUnit(id_, rotation_ + rotation_offset);
  }
}

void T34Tank::RotateTurret() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input = player->GetInputData();
    auto diff = input.mouse_cursor_position - position_;
    turret_rotation_ = glm::length(diff) < 1e-4 ? rotation_ : std::atan2(diff.y, diff.x) - glm::radians(90.0f);
  }
}

void T34Tank::Fire() {
  if (fire_count_down_ == 0) {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input = player->GetInputData();
      if (input.mouse_button_down[GLFW_MOUSE_BUTTON_LEFT]) {
        auto velocity = Rotate(glm::vec2{0.0f, 20.0f}, turret_rotation_);
        GenerateBullet<bullet::CannonBall>(position_ + Rotate({0.0f, 1.2f}, turret_rotation_), turret_rotation_, GetDamageScale(), velocity);
        fire_count_down_ = kTickPerSecond;
      }
    }
  }
  if (fire_count_down_) fire_count_down_--;
}

void T34Tank::UseSpeedBoost() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input = player->GetInputData();
    if (speed_boost_cooldown_ == 0 && input.key_down[GLFW_KEY_SPACE]) {
      is_speed_boost_active_ = true;
      speed_boost_timer_ = 3 * kTickPerSecond;  
      speed_boost_cooldown_ = 5 * kTickPerSecond;  
    }
  }

  if (is_speed_boost_active_) {
    if (speed_boost_timer_ > 0) {
      speed_boost_timer_--;
    } else {
      is_speed_boost_active_ = false;
    }
  }

  if (speed_boost_cooldown_ > 0) {
    speed_boost_cooldown_--;
  }
}

bool T34Tank::IsHit(glm::vec2 position) const {
  position = WorldToLocal(position);
  return position.x > -0.8f && position.x < 0.8f && position.y > -1.0f && position.y < 1.0f;
}

const char *T34Tank::UnitName() const { return "T34 Tank with Speed Boost"; }
const char *T34Tank::Author() const { return "58876"; }

}  
