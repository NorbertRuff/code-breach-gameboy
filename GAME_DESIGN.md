# ApexLab: Code Breach - Game Design Document

## 🎮 Game Concept
**ApexLab: Code Breach** is a platformer game where you play as a programmer at ApexLab (a programming company) who must escape when the AI systems go haywire! The lab's security protocols have been corrupted, and you need to escape while fixing code bugs and avoiding digital threats.

## 🏢 Story & Setting
- **Setting**: ApexLab programming facility
- **Protagonist**: ApexLab programmer/employee
- **Conflict**: AI security breach, corrupted systems
- **Goal**: Escape the lab while collecting code fragments and avoiding digital threats
- **Theme**: Programming/tech, cybersecurity, AI gone wrong

## 🎯 Core Gameplay
- **Genre**: 2D Platformer with collectibles
- **Movement**: Left/right movement, jumping
- **Objective**: Navigate through lab sections, collect items, reach exit
- **Challenge**: Avoid enemies, navigate obstacles, solve simple puzzles

## 🎨 Visual Style
- **Aesthetic**: Cyberpunk/tech with programming elements
- **Colors**: Blues, greens, glowing effects
- **Elements**: Circuits, code symbols, digital effects
- **Environment**: Modern lab with computers, servers, testing equipment

## 🗺️ Level Design

### Level 1: Server Room
- **Theme**: Server racks, cables, glowing lights
- **Enemies**: Corrupted security bots
- **Collectibles**: Code fragments, security keys
- **Obstacles**: Broken servers, exposed wiring

### Level 2: Code Testing Lab
- **Theme**: Computer workstations, code displays
- **Enemies**: Digital viruses, corrupted code blocks
- **Collectibles**: Bug reports, debug tools
- **Obstacles**: Floating code fragments, broken terminals

### Level 3: AI Research Wing
- **Theme**: Advanced AI systems, neural network displays
- **Enemies**: Malfunctioning AI entities
- **Collectibles**: AI cores, security protocols
- **Obstacles**: Digital barriers, corrupted data streams

### Level 4: Network Hub
- **Theme**: Network infrastructure, data flow
- **Enemies**: Network security bots, data corruption
- **Collectibles**: Network keys, data packets
- **Obstacles**: Broken connections, firewall barriers

### Level 5: Mainframe (Final Level)
- **Theme**: Core system, central processing
- **Enemies**: Final AI boss, security systems
- **Collectibles**: Master key, system reset code
- **Obstacles**: Ultimate challenge, final escape

## 🎭 Character Design
- **Main Character**: ApexLab programmer
  - **Appearance**: Lab coat, glasses, tech accessories
  - **Animations**: Walk, run, jump, collect items
  - **Size**: 16x16 or 16x32 sprite
- **Enemies**:
  - Security bots (patrolling)
  - Digital viruses (chasing)
  - Corrupted code blocks (moving obstacles)
  - AI entities (intelligent threats)

## 🎮 Gameplay Mechanics

### Core Movement
- [x] Left/right movement
- [ ] Jumping mechanics
- [ ] Smooth acceleration/deceleration
- [ ] Wall collision detection

### Collectibles
- [ ] Code fragments (basic collectibles)
- [ ] Bug reports (special items)
- [ ] Security keys (unlock areas)
- [ ] Debug tools (power-ups)

### Enemies & AI
- [ ] Basic enemy movement patterns
- [ ] Collision detection with enemies
- [ ] Enemy spawning system
- [ ] Different enemy types

### Level Progression
- [ ] Level completion triggers
- [ ] Score tracking
- [ ] Lives system
- [ ] Level transitions

## 📋 Implementation Plan

### Phase 1: Foundation (Week 1-2)
- [x] Replace Mario character with programmer sprite
- [ ] Implement jumping mechanics
- [ ] Add basic collision detection
- [ ] Create new background assets
- [ ] Implement basic enemy system

### Phase 2: Core Gameplay (Week 3-4)
- [ ] Design and implement first level (Server Room)
- [ ] Add collectible system
- [ ] Implement enemy AI patterns
- [ ] Add sound effects
- [ ] Create HUD for score/lives

### Phase 3: Level Expansion (Week 5-6)
- [ ] Design remaining levels
- [ ] Implement level transitions
- [ ] Add power-up system
- [ ] Create boss encounters
- [ ] Implement save/load system

### Phase 4: Polish & Testing (Week 7-8)
- [ ] Add background music
- [ ] Implement screen transitions
- [ ] Add particle effects
- [ ] Playtesting and bug fixes
- [ ] Performance optimization

## 🛠️ Technical Requirements

### Graphics
- [ ] Programmer character sprites (walk, run, jump)
- [ ] Enemy sprites (bots, viruses, AI entities)
- [ ] Background tilesets for each level
- [ ] UI elements (HUD, menus, transitions)
- [ ] Collectible item sprites

### Audio
- [ ] Background music for each level
- [ ] Sound effects (jump, collect, enemy, death)
- [ ] Menu navigation sounds

### Code Architecture
- [ ] Modular level system
- [ ] Enemy AI state machine
- [ ] Collectible management system
- [ ] Level progression manager
- [ ] Save/load system

## 🧪 Development Features & Tools

### Feature Flags
- [x] **SKIP_INTRO_AND_START**: Skip intro credits and press start screen for faster development/testing
  - Set to `1` in `src/utils.h` to enable
  - Set to `0` to restore normal intro sequence
  - Useful for rapid iteration during development

- [x] **EMPTY_BACKGROUND**: Draw empty background instead of city background for debugging
  - Set to `1` in `src/utils.h` to enable
  - Set to `0` to restore normal city background
  - Useful for debugging character movement and collision detection

- [x] **DISABLE_BACKGROUND_SCROLLING**: Disable background scrolling for debugging
  - Set to `1` in `src/utils.h` to enable
  - Set to `0` to restore normal background scrolling
  - Useful for debugging character movement without background interference

### Debug Tools
- [ ] Frame rate display
- [ ] Collision box visualization
- [ ] Enemy spawn indicators
- [ ] Performance monitoring

### Testing Utilities
- [ ] Level skip commands
- [ ] Invincibility toggle
- [ ] Enemy spawn controls
- [ ] Save state management

## 🎯 Success Metrics
- **Gameplay**: Smooth 60fps performance
- **Content**: 5 unique levels with increasing difficulty
- **Replayability**: Collectible completion, speed runs
- **Polish**: Professional feel, smooth transitions
- **Fun Factor**: Engaging gameplay loop

## 🚀 Future Enhancements
- **Multiplayer**: Co-op escape mode
- **Level Editor**: Create custom levels
- **Achievements**: Unlockable content
- **Soundtrack**: Extended music collection
- **Mobile Port**: Adapt for mobile devices

---

*This document will be updated as development progresses.*
*Last updated: [Current Date]*
