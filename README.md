# Snake Arena

Snake Arena is a web-based Snake game built using React, Vite, Node.js, and Express. It includes user authentication, leaderboard tracking, multiple game modes, and mobile-friendly controls.

## Features

- User registration and login
- Persistent leaderboard with high scores
- Classic and Wrap game modes
- Easy, Normal, and Hard difficulty levels
- Bonus food and obstacles
- Mobile touch controls
- Pause and resume support

## Project Contents

- Web game (React + Vite frontend, Express backend)
- Desktop C source: `Snake1.c`

## Live Demo

https://snake-arena1.onrender.com/

> Note: The demo is hosted on Render's free tier and may take 20–30 seconds to wake up after inactivity.

---

## Gameplay Preview

![Snake Arena Gameplay]
<img width="1091" height="921" alt="image" src="https://github.com/user-attachments/assets/d8f68089-1121-4b72-ac1b-9d3594837b8f" />

---

## Run the Project Locally

From the `web` folder:

```bash
npm install
```

Start the backend:

```bash
npm start
```

Open another terminal and start the frontend:

```bash
npm run dev
```

Open the application:

Frontend:

```
http://localhost:5173
```

Backend Health Check:

```
http://localhost:3000/api/health
```

## Controls

- Arrow Keys or WASD
- On-screen touch controls (Mobile)
- Press any key or tap the game board to start

## Run the Desktop C Version

From the repository root:

```bash
gcc Snake1.c -o Snake1.exe
./Snake1.exe
```

If `gcc` is not recognized, install MinGW and add it to your system PATH.

## Tech Stack

- Frontend: React, Vite
- Backend: Node.js, Express
- Storage: JSON (`data/users.json`)

## Author

Akash MP