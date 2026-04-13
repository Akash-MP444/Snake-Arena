# SNAKE ARENA

Fast-paced snake game with accounts, leaderboard tracking, multiple game modes, and mobile support.

## Features

- Login and register system
- Leaderboard with saved high scores
- Clear scores / clear all users (admin actions)
- Classic mode and wrap mode
- Difficulty levels
- Bonus food and obstacle mechanics
- Mobile-friendly touch controls

## Project Contents

- Web game (frontend + backend)
- Desktop C source: `Snake1.c`

## Live Demo

https://snake-arena1.onrender.com/

Note: The demo is hosted on a free tier and may take around 20-30 seconds to wake up.

## Run the Web Game Locally

From the `web` folder:

```powershell
npm install
npm start
```

Open in browser:

http://localhost:3000

## Controls

- Desktop: Arrow keys or WASD
- Mobile: On-screen touch buttons
- Start: Press any key or tap the game board

## Run the Desktop C Version

From the repository root:

```powershell
gcc .\Snake1.c -o .\Snake1_v3.exe
.\Snake1_v3.exe
```

If `gcc` is not recognized, install MinGW and add it to PATH.

## Tech Stack

- Frontend: HTML, CSS, JavaScript
- Backend: Node.js, Express
- Storage: JSON file-based data (`data/users.json`)

## Author

Akash MP
