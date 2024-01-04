/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Simon Sandström
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "game_ui.h"

#include <cstdint>
#include <variant>
#include <SDL2/SDL.h>

#include "utils/data_loader.h"
#include "bitmap_font.h"
#include "common_ui.h"
#include "game.h"
#include "texture.h"

namespace
{

struct RenderedCreature
{
  std::string name;
  int health_percentage;
  common::Position local_position;
};

std::vector<RenderedCreature> rendered_creatures;

}

namespace game
{

using ui::common::BLACK;

GameUI::GameUI(const game::Game* game,
               SDL_Renderer* renderer,
               const SpriteLoader* sprite_loader,
               const utils::data_loader::ItemTypes* item_types)
    : m_game(game),
      m_renderer(renderer),
      m_sprite_loader(sprite_loader),
      m_item_types(item_types),
      m_texture(nullptr, SDL_DestroyTexture),
      m_scaled_texture(nullptr, SDL_DestroyTexture),
      m_anim_tick(0u),
      m_creature_textures(),
      m_item_textures()
{
  m_texture.reset(SDL_CreateTexture(m_renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    TEXTURE_WIDTH,
                                    TEXTURE_HEIGHT));
  m_scaled_texture.reset(SDL_CreateTexture(m_renderer,
                                           SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET,
                                           TEXTURE_WIDTH * SCALING,
                                           TEXTURE_HEIGHT * SCALING));
}

SDL_Texture* GameUI::render()
{
  m_anim_tick = SDL_GetTicks() / 540;

  SDL_SetRenderTarget(m_renderer, m_scaled_texture.get());
  SDL_SetRenderDrawColor(m_renderer, 0U, 0U, 0U, 255U);
  SDL_RenderClear(m_renderer);

  if (!m_game->ready())
  {
    return m_scaled_texture.get();
  }

  rendered_creatures.clear();

  SDL_SetRenderTarget(m_renderer, m_texture.get());
  SDL_SetRenderDrawColor(m_renderer, 0U, 0U, 0U, 255U);
  SDL_RenderClear(m_renderer);

  // Render tiles and things
  if (m_game->getPlayerPosition().getZ() <= 7)
  {
    // We have floors: 7  6  5  4  3  2  1  0, and we want to render them in that order
    // Skip floors above player z as we don't know when floor above blocks view of player yet
    for (auto z = 0; z <= 7; ++z)
    {
      renderFloor(z);
      if (7 - m_game->getPlayerPosition().getZ() == z)
      {
        break;
      }
    }
  } else
  {
    // Underground, render at the bottom floor up to player floor (which is always local z=2)
    for (auto z = m_game->getNumFloors() - 1; z >= 2; --z)
    {
      renderFloor(z);
    }
  }

  // Render to scaled texture
  SDL_SetRenderTarget(m_renderer, m_scaled_texture.get());
  SDL_Rect scaled_rect = { 0, 0, static_cast<int>(TEXTURE_WIDTH * SCALING), static_cast<int>(TEXTURE_HEIGHT * SCALING) };
  SDL_RenderCopy(m_renderer, m_texture.get(), nullptr, &scaled_rect);

  // Render creature names and health bar to scaled texture
  for (const auto& rendered_creature : rendered_creatures)
  {
    SDL_Color creature_status_color;
    if (rendered_creature.health_percentage > 92)
    {
      creature_status_color = { 0U, 188U, 0U, 255U };
    }
    else if (rendered_creature.health_percentage > 60)
    {
      creature_status_color = { 80U, 161U, 80U, 255U };
    }
    else if (rendered_creature.health_percentage > 30)
    {
      creature_status_color = { 161U, 161U, 0U, 255U };
    }
    else if (rendered_creature.health_percentage > 8)
    {
      creature_status_color = { 191U, 10U, 10U, 255U };
    }
    else if (rendered_creature.health_percentage > 3)
    {
      creature_status_color = { 145U, 15U, 15U, 255U };
    }
    else
    {
      creature_status_color = { 133U, 12U, 12U, 255U };
    }

    ui::common::get_bitmap_font()->renderText((rendered_creature.local_position.getX() * TILE_SIZE * SCALING) + 16,
                                              (rendered_creature.local_position.getY() * TILE_SIZE * SCALING) - 30,
                                              rendered_creature.name,
                                              creature_status_color,
                                              true);

    const SDL_Rect health_bar_base_rect
    {
      static_cast<int>(rendered_creature.local_position.getX() * TILE_SIZE * SCALING) - 0,
      static_cast<int>(rendered_creature.local_position.getY() * TILE_SIZE * SCALING) - 17,
      27,
      4
    };
    SDL_SetRenderDrawColor(m_renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
    SDL_RenderFillRect(m_renderer, &health_bar_base_rect);

    const SDL_Rect health_bar_rect
    {
      health_bar_base_rect.x + 1,
      health_bar_base_rect.y + 1,
      // full width is 25, e.g. 1/4 of 100
      // so for each 4% hp lost, we reduce the width by 1
      health_bar_base_rect.w - 2 - ((100 - rendered_creature.health_percentage) / 4),
      health_bar_base_rect.h - 2
    };
    SDL_SetRenderDrawColor(m_renderer, creature_status_color.r, creature_status_color.g, creature_status_color.b, 255U);
    SDL_RenderFillRect(m_renderer, &health_bar_rect);
  }

  // Render static texts
  for (const auto& static_text : m_game->getStaticTexts())
  {
    const auto& local_position = m_game->globalToLocalPosition(static_text.position);
    std::string text = "";
    SDL_Color color;
    switch (static_text.type)
    {
      case 1:
      {
        text += static_text.talker + " says: " + static_text.text;
        color = { 239U, 239U, 0U, 255U };
        break;
      }
      case 2:
      {
        text += static_text.talker + " whispers: " + static_text.text;
        color = { 239U, 239U, 0U, 255U };
        break;
      }
      case 3:  // yell
      {
        text += static_text.talker + " yells: " + static_text.text;
        color = { 239U, 239U, 0U, 255U };
        break;
      }
      case 16:
      case 17:
      {
        text = static_text.text;
        color = { 254U, 101U, 0U, 0U };
        break;
      }
    }

    // TODO: handle text off-screen
    ui::common::get_bitmap_font()->renderText((local_position.getX() * TILE_SIZE * SCALING) + 16,
                                              (local_position.getY() * TILE_SIZE * SCALING) - 30,
                                              text,
                                              color,
                                              false);
  }

  return m_scaled_texture.get();
}

void GameUI::onClick(int x, int y)
{
  // note: z is not set by screenToMapPosition
  const auto local_tile_x = static_cast<std::uint16_t>(x / TILE_SIZE);
  const auto local_tile_y = static_cast<std::uint16_t>(y / TILE_SIZE);
  LOG_INFO("%s: local_tile: %d,%d", __func__, local_tile_x, local_tile_y);

  // Convert to global position
  const auto& player_position = m_game->getPlayerPosition();
  const auto global_position = common::Position(player_position.getX() - 7 + local_tile_x,
                                                player_position.getY() - 5 + local_tile_y,
                                                player_position.getZ());

  const auto* tile = m_game->getTile(global_position);
  if (tile)
  {
    LOG_INFO("Tile at %s", global_position.toString().c_str());
    int stackpos = 0;
    for (const auto& thing : tile->things)
    {
      if (std::holds_alternative<game::Item>(thing))
      {
        const auto& item = std::get<game::Item>(thing);
        std::ostringstream oss;
        oss << "  stackpos=" << stackpos << " ";
        item.type->dump(&oss, false);
        oss << " [extra=" << static_cast<int>(item.extra) << "]\n";
        LOG_INFO(oss.str().c_str());
      }
      else if (std::holds_alternative<common::CreatureId>(thing))
      {
        const auto creature_id = std::get<common::CreatureId>(thing);
        const auto* creature = m_game->getCreature(creature_id);
        if (creature)
        {
          LOG_INFO("  stackpos=%d Creature [id=%d, name=%s]", stackpos, creature_id, creature->name.c_str());
        }
        else
        {
          LOG_ERROR("  stackpos=%d: creature with id=%d is nullptr", stackpos, creature_id);
        }
      }
      else
      {
        LOG_ERROR("  stackpos=%d: invalid Thing on Tile", stackpos);
      }

      ++stackpos;
    }
  }
  else
  {
    LOG_ERROR("%s: clicked on invalid tile", __func__);
  }
}

void GameUI::renderFloor(int z)
{
  auto it = m_game->getTiles().cbegin() + (z * game::KNOWN_TILES_X * game::KNOWN_TILES_Y);

  // Skip first row
  it += game::KNOWN_TILES_X;

  for (auto y = 0; y < DRAW_TILES_Y + 1; y++)
  {
    // Skip first column
    ++it;

    for (auto x = 0; x < DRAW_TILES_X + 1; x++)
    {
      renderTile(x, y, z, *it);
      ++it;
    }

    // Skip the 2nd extra column to the right
    ++it;
  }
}

void GameUI::renderTile(int x, int y, int z, const game::Tile& tile)
{
  if (tile.things.empty())
  {
    return;
  }

  // Set hangable hook side
  auto hook_side = HangableHookSide::NONE;
  for (const auto& thing : tile.things)
  {
    if (std::holds_alternative<game::Item>(thing))
    {
      const auto& item = std::get<game::Item>(thing);
      if (item.type->is_hook_east)
      {
        hook_side = HangableHookSide::EAST;
        break;
      }

      if (item.type->is_hook_south)
      {
        hook_side = HangableHookSide::SOUTH;
        break;
      }
    }
  }

  // Order:
  // 1. Bottom items (ground, on_bottom)
  // 2. Common items in reverse order (neither creature, on_bottom nor on_top)
  // 3. Creatures (reverse order?)
  // 4. (Effects)
  // 5. Top items (on_top)

  // Keep track of elevation
  auto elevation = 0;

  // Draw ground and on_bottom items
  for (const auto& thing : tile.things)
  {
    if (std::holds_alternative<game::Item>(thing))
    {
      const auto& item = std::get<game::Item>(thing);
      if (item.type->is_ground || item.type->is_on_bottom)
      {
        renderItem(x, y, item, hook_side, elevation);
        elevation += item.type->elevation;
        continue;
      }
    }
    break;
  }

  // Draw items, neither on_bottom nor on_top, in reverse order
  for (auto it = tile.things.rbegin(); it != tile.things.rend(); ++it)
  {
    if (std::holds_alternative<game::Item>(*it))
    {
      const auto& item = std::get<game::Item>(*it);
      if (!item.type->is_ground && !item.type->is_on_top && !item.type->is_on_bottom)
      {
        renderItem(x, y, item, hook_side, elevation);
        elevation += item.type->elevation;
        continue;
      }

      if (item.type->is_on_top)
      {
        // to not hit the break below as there can be items left to render here
        continue;
      }
    }
    break;
  }

  // Draw creatures, in reverse order
  for (auto it = tile.things.rbegin(); it != tile.things.rend(); ++it)
  {
    if (std::holds_alternative<common::CreatureId>(*it))
    {
      const auto creature_id = std::get<common::CreatureId>(*it);
      const auto* creature = m_game->getCreature(creature_id);
      if (creature)
      {
        renderCreature(x, y, *creature, elevation);
        if (m_game->getPlayerLocalZ() == z)
        {
          rendered_creatures.push_back({ creature->name, creature->health_percent, common::Position(x, y, z) });
        }
      }
      else
      {
        LOG_ERROR("%s: cannot render creature with id %u, no creature data",
                  __func__,
                  creature_id);
      }
    }
  }

  // Draw on_top items
  for (const auto& thing : tile.things)
  {
    if (std::holds_alternative<game::Item>(thing))
    {
      const auto& item = std::get<game::Item>(thing);
      if (item.type->is_on_top)
      {
        renderItem(x, y, item, hook_side, elevation);
        elevation += item.type->elevation;
        continue;
      }
    }
    break;
  }
}

void GameUI::renderItem(int x, int y, const game::Item& item, HangableHookSide hook_side, std::uint16_t elevation)
{
  if (item.type->type != common::ItemType::Type::ITEM)
  {
    LOG_ERROR("%s: called but item type: %u is not an item", __func__, item.type->id);
    return;
  }

  if (item.type->id == 0)
  {
    return;
  }

  auto* sdl_texture = getItemSDLTexture(x, y, item, hook_side);

  // TODO(simon): there is probably a max offset...
  const SDL_Rect dest
  {
    (x * TILE_SIZE - elevation - (item.type->is_displaced ? 8 : 0) - ((item.type->sprite_info.width - 1) * 32)),
    (y * TILE_SIZE - elevation - (item.type->is_displaced ? 8 : 0) - ((item.type->sprite_info.height - 1) * 32)),
    item.type->sprite_info.width * TILE_SIZE,
    item.type->sprite_info.height * TILE_SIZE
  };
  SDL_RenderCopy(m_renderer, sdl_texture, nullptr, &dest);
}

void GameUI::renderCreature(int x, int y, const game::Creature& creature, std::uint16_t offset)
{
  if (creature.outfit.type == 0U)
  {
    // note: if both are zero then creature is invis
    if (creature.outfit.item_id != 0U)
    {
      const auto& item_type = (*m_item_types)[creature.outfit.item_id];
      game::Item item = { &item_type, 0U };
      renderItem(x, y, item, HangableHookSide::NONE, 0);
    }
    return;
  }

  const auto* texture = getCreatureTexture(creature.id);
  if (!texture)
  {
    return;
  }
  auto* sdl_texture = texture->getCreatureStillTexture(creature.direction);
  if (!sdl_texture)
  {
    return;
  }

  const SDL_Rect dest
  {
    (x * TILE_SIZE - offset - 8),
    (y * TILE_SIZE - offset - 8),
    TILE_SIZE,
    TILE_SIZE
  };
  SDL_RenderCopy(m_renderer, sdl_texture, nullptr, &dest);
}

SDL_Texture* GameUI::getItemSDLTexture(int x, int y, const game::Item& item, HangableHookSide hook_side)
{
  const auto& texture = getItemTexture(item.type->id);

  auto version = 0;
  if ((item.type->is_fluid_container || item.type->is_splash) && item.extra < texture.getNumVersions())
  {
    version = item.extra;
  }
  else if (item.type->is_stackable)
  {
    // index 0 -> count 1
    // index 1 -> count 2
    // index 2 -> count 3
    // index 3 -> count 4
    // index 4 -> count 5
    // index 5 -> count 6..10 (?)
    // index 6 -> count 11..25 (?)
    // index 7 -> count 25..100 (?)
    if (item.extra <= 5U)
    {
      version = item.extra - 1;
    }
    else if (item.extra <= 10U)
    {
      version = 5;
    }
    else if (item.extra <= 25U)
    {
      version = 6;
    }
    else
    {
      version = 7;
    }

    // Some item have less than 8 sprites for different stack/count
    version = std::min(version, texture.getNumVersions() - 1);
  }
  else if (item.type->is_hangable && texture.getNumVersions() == 3)
  {
    switch (hook_side)
    {
      case HangableHookSide::NONE:
        version = 0;
        break;

      case HangableHookSide::SOUTH:
        version = 1;
        break;

      case HangableHookSide::EAST:
        version = 2;
        break;
    }
  }
  else
  {
    // TODO(simon): need to use world position, not local position
    version = ((y % item.type->sprite_info.ydiv) * item.type->sprite_info.xdiv) + (x % item.type->sprite_info.xdiv);
  }

  return texture.getItemTexture(version, m_anim_tick);
}

const Texture& GameUI::getItemTexture(common::ItemTypeId item_type_id)
{
  auto it = std::find_if(m_item_textures.cbegin(),
                         m_item_textures.cend(),
                         [&item_type_id](const Texture& item_texture)
                         {
                           return item_texture.getItemTypeId() == item_type_id;
                         });

  // Create textures if not found
  if (it == m_item_textures.cend())
  {
    const auto& item_type = (*m_item_types)[item_type_id];
    m_item_textures.push_back(Texture::createItemTexture(m_renderer, *m_sprite_loader, item_type));
    it = m_item_textures.end() - 1;
  }

  return *it;
}

const Texture* GameUI::getCreatureTexture(common::CreatureId creature_id)
{
  auto it = std::find_if(m_creature_textures.cbegin(),
                         m_creature_textures.cend(),
                         [&creature_id](const CreatureTexture& creature_texture)
                         {
                           return creature_texture.creature_id == creature_id;
                         });
  // Create textures if not found
  if (it == m_creature_textures.cend())
  {
    const auto* creature = m_game->getCreature(creature_id);
    const auto& item_type = (*m_item_types)[3134 + creature->outfit.type];
    m_creature_textures.emplace_back();
    m_creature_textures.back().creature_id = creature_id;
    m_creature_textures.back().texture = Texture::createOutfitTexture(m_renderer, *m_sprite_loader, item_type, creature->outfit);
    it = m_creature_textures.end() - 1;
  }
  // TODO: remove texture when creature removed from knownCreatures..?
  return &(it->texture);
}

}  // namespace game