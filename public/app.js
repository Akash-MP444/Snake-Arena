const { useEffect, useMemo, useRef, useState } = React;

const GRID = 20;
const CELL = 22;
const SCORE_PER_FOOD = 10;
const BONUS_SCORE = 25;
const FOODS_PER_LEVEL = 5;
const BONUS_SPAWN_INTERVAL = 3;
const BONUS_LIFETIME_TICKS = 35;
const BASE_DELAY = 220;
const LEVEL_SPEEDUP = 12;
const MIN_DELAY = 70;

const DIR = {
  UP: { x: 0, y: -1 },
  DOWN: { x: 0, y: 1 },
  LEFT: { x: -1, y: 0 },
  RIGHT: { x: 1, y: 0 }
};

function keyOf(p) {
  return `${p.x},${p.y}`;
}

function randomPos(blocked) {
  while (true) {
    const p = { x: Math.floor(Math.random() * GRID), y: Math.floor(Math.random() * GRID) };
    if (!blocked.has(keyOf(p))) return p;
  }
}

function makeObstacles(level, blocked) {
  const count = Math.min(level - 1, 40);
  const arr = [];
  while (arr.length < count) {
    const p = { x: Math.floor(Math.random() * GRID), y: Math.floor(Math.random() * GRID) };
    if (Math.abs(p.x - GRID / 2) <= 1 && Math.abs(p.y - GRID / 2) <= 1) continue;
    if (blocked.has(keyOf(p))) continue;
    blocked.add(keyOf(p));
    arr.push(p);
  }
  return arr;
}

function App() {
  const canvasRef = useRef(null);
  const [tab, setTab] = useState("login");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const [user, setUser] = useState(null);
  const [error, setError] = useState("");
  const [leaderboard, setLeaderboard] = useState([]);

  const [mode, setMode] = useState("classic");
  const [difficulty, setDifficulty] = useState("normal");
  const [running, setRunning] = useState(false);
  const [paused, setPaused] = useState(false);
  const [awaitingStart, setAwaitingStart] = useState(true);
  const [gameOver, setGameOver] = useState(false);

  const [snake, setSnake] = useState([{ x: 10, y: 10 }]);
  const [dir, setDir] = useState(DIR.RIGHT);
  const [food, setFood] = useState({ x: 14, y: 10 });
  const [bonus, setBonus] = useState(null);
  const [bonusTimer, setBonusTimer] = useState(0);
  const [obstacles, setObstacles] = useState([]);
  const [score, setScore] = useState(0);
  const [highScore, setHighScore] = useState(0);
  const [foodsEaten, setFoodsEaten] = useState(0);
  const [level, setLevel] = useState(1);

  const speedModifier = difficulty === "easy" ? 45 : difficulty === "hard" ? -35 : 0;
  const frameDelay = Math.max(MIN_DELAY, BASE_DELAY - (level - 1) * LEVEL_SPEEDUP + speedModifier);
  const canSubmitRegister = username.trim().length >= 3 && password.length >= 4;

  const obstacleSet = useMemo(() => new Set(obstacles.map(keyOf)), [obstacles]);

  const changeDirection = (nextDir) => {
    setDir((d) => {
      if (nextDir === DIR.UP && d === DIR.DOWN) return d;
      if (nextDir === DIR.DOWN && d === DIR.UP) return d;
      if (nextDir === DIR.LEFT && d === DIR.RIGHT) return d;
      if (nextDir === DIR.RIGHT && d === DIR.LEFT) return d;
      return nextDir;
    });
  };

  const resetGame = () => {
    const baseSnake = [{ x: 10, y: 10 }];
    const blocked = new Set(baseSnake.map(keyOf));
    const nextObstacles = makeObstacles(1, blocked);
    nextObstacles.forEach((o) => blocked.add(keyOf(o)));

    setSnake(baseSnake);
    setDir(DIR.RIGHT);
    setScore(0);
    setFoodsEaten(0);
    setLevel(1);
    setBonus(null);
    setBonusTimer(0);
    setObstacles(nextObstacles);
    setFood(randomPos(blocked));
    setPaused(false);
    setRunning(false);
    setGameOver(false);
    setAwaitingStart(true);
  };

  const startRound = (freshRound) => {
    if (!user) return;
    if (freshRound) {
      resetGame();
    }
    setPaused(false);
    setGameOver(false);
    setAwaitingStart(false);
    setRunning(true);
  };

  const onDirectionPress = (nextDir) => {
    if (!user) return;
    if (!running) {
      startRound(gameOver);
    }
    changeDirection(nextDir);
  };

  const onAnyStart = () => {
    if (!user || running) return;
    startRound(gameOver);
  };

  const draw = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#030712";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = "#a855f7";
    obstacles.forEach((o) => ctx.fillRect(o.x * CELL, o.y * CELL, CELL - 2, CELL - 2));

    ctx.fillStyle = "#ef4444";
    ctx.fillRect(food.x * CELL, food.y * CELL, CELL - 2, CELL - 2);

    if (bonus) {
      ctx.fillStyle = "#f59e0b";
      ctx.fillRect(bonus.x * CELL, bonus.y * CELL, CELL - 2, CELL - 2);
    }

    snake.forEach((s, i) => {
      ctx.fillStyle = i === 0 ? "#22c55e" : "#16a34a";
      ctx.fillRect(s.x * CELL, s.y * CELL, CELL - 2, CELL - 2);
    });
  };

  async function refreshLeaderboard() {
    const res = await fetch("/api/leaderboard");
    setLeaderboard(await res.json());
  }

  async function auth(endpoint) {
    setError("");
    const res = await fetch(`/api/${endpoint}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password })
    });
    const data = await res.json();
    if (!res.ok) {
      setError(data.error || "Request failed.");
      return;
    }
    setUser(data.username);
    setHighScore(data.highScore || 0);
    setTab("game");
    resetGame();
    await refreshLeaderboard();
  }

  async function pushScore(finalScore) {
    const res = await fetch("/api/score", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username: user, score: finalScore })
    });
    const data = await res.json();
    if (res.ok) {
      setHighScore(data.highScore || 0);
      await refreshLeaderboard();
    }
  }

  async function clearLeaderboardScores() {
    const ok = window.confirm("Clear all scores but keep users?");
    if (!ok) return;
    const res = await fetch("/api/leaderboard/clear-scores", { method: "POST" });
    const data = await res.json();
    if (!res.ok || !data.ok) {
      setError("Could not clear scores.");
      return;
    }
    if (user) setHighScore(0);
    await refreshLeaderboard();
  }

  async function clearAllUsers() {
    const ok = window.confirm("Delete all users and clear leaderboard?");
    if (!ok) return;
    const res = await fetch("/api/leaderboard/clear", { method: "POST" });
    const data = await res.json();
    if (!res.ok || !data.ok) {
      setError("Could not clear users.");
      return;
    }
    setRunning(false);
    setPaused(false);
    setUser(null);
    setHighScore(0);
    setUsername("");
    setPassword("");
    setError("");
    await refreshLeaderboard();
  }

  useEffect(() => {
    refreshLeaderboard();
  }, []);

  useEffect(() => {
    draw();
  }, [snake, food, bonus, obstacles]);

  useEffect(() => {
    const onKey = (e) => {
      if (!user) return;

      if (!running) {
        startRound(gameOver);
      }

      if ((e.key === "p" || e.key === "P") && running) {
        setPaused((v) => !v);
        return;
      }

      if (e.key === "ArrowUp" || e.key === "w" || e.key === "W") {
        changeDirection(DIR.UP);
      } else if (e.key === "ArrowDown" || e.key === "s" || e.key === "S") {
        changeDirection(DIR.DOWN);
      } else if (e.key === "ArrowLeft" || e.key === "a" || e.key === "A") {
        changeDirection(DIR.LEFT);
      } else if (e.key === "ArrowRight" || e.key === "d" || e.key === "D") {
        changeDirection(DIR.RIGHT);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [user, running, gameOver]);

  useEffect(() => {
    if (!running || paused || !user) return;

    const id = setInterval(() => {
      setSnake((prev) => {
        const head = { x: prev[0].x + dir.x, y: prev[0].y + dir.y };

        if (mode === "wrap") {
          if (head.x < 0) head.x = GRID - 1;
          if (head.x >= GRID) head.x = 0;
          if (head.y < 0) head.y = GRID - 1;
          if (head.y >= GRID) head.y = 0;
        }

        if (
          (mode === "classic" && (head.x < 0 || head.y < 0 || head.x >= GRID || head.y >= GRID)) ||
          obstacleSet.has(keyOf(head)) ||
          prev.some((p) => p.x === head.x && p.y === head.y)
        ) {
          setRunning(false);
          setGameOver(true);
          setAwaitingStart(true);
          pushScore(score);
          return prev;
        }

        const next = [head, ...prev];
        let ateFood = false;

        if (head.x === food.x && head.y === food.y) {
          ateFood = true;
          const nextFoods = foodsEaten + 1;
          const nextScore = score + SCORE_PER_FOOD;
          setFoodsEaten(nextFoods);
          setScore(nextScore);

          const blocked = new Set(next.map(keyOf));
          obstacles.forEach((o) => blocked.add(keyOf(o)));
          if (bonus) blocked.add(keyOf(bonus));
          setFood(randomPos(blocked));

          if (nextFoods % FOODS_PER_LEVEL === 0) {
            const nextLevel = level + 1;
            setLevel(nextLevel);
            const obstacleBlocked = new Set(next.map(keyOf));
            const newObstacles = makeObstacles(nextLevel, obstacleBlocked);
            setObstacles(newObstacles);
          }

          if (nextFoods % BONUS_SPAWN_INTERVAL === 0 && !bonus) {
            const bonusBlocked = new Set(next.map(keyOf));
            obstacles.forEach((o) => bonusBlocked.add(keyOf(o)));
            bonusBlocked.add(keyOf(food));
            setBonus(randomPos(bonusBlocked));
            setBonusTimer(BONUS_LIFETIME_TICKS);
          }
        }

        if (bonus && head.x === bonus.x && head.y === bonus.y) {
          setScore((v) => v + BONUS_SCORE);
          setBonus(null);
          setBonusTimer(0);
        }

        if (!ateFood) next.pop();
        return next;
      });

      if (bonus) {
        setBonusTimer((t) => {
          if (t <= 1) {
            setBonus(null);
            return 0;
          }
          return t - 1;
        });
      }
    }, frameDelay);

    return () => clearInterval(id);
  }, [running, paused, user, dir, mode, food, bonus, obstacleSet, obstacles, foodsEaten, level, frameDelay, score]);

  return (
    <div className="app">
      <div className="hero card">
        <h1 className="title-center">SNAKE ARENA</h1>
      </div>

      {!user && (
        <div className="card">
          <h3>{tab === "login" ? "Login" : "Register"}</h3>
          <div className="row auth-row">
            <input value={username} onChange={(e) => setUsername(e.target.value)} placeholder="Username" />
            <div className="password-wrap">
              <input
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                type={showPassword ? "text" : "password"}
                placeholder="Password"
              />
              <button
                type="button"
                className="eye-btn"
                onClick={() => setShowPassword((v) => !v)}
                aria-label={showPassword ? "Hide password" : "Show password"}
                title={showPassword ? "Hide password" : "Show password"}
              >
                {showPassword ? "🙈" : "👁"}
              </button>
            </div>
            {tab === "login" ? (
              <button onClick={() => auth("login")}>Login</button>
            ) : (
              <button onClick={() => auth("register")} disabled={!canSubmitRegister}>Register</button>
            )}
          </div>
          <button className="switch-auth" onClick={() => setTab(tab === "login" ? "register" : "login")}>
            Switch to {tab === "login" ? "Register" : "Login"}
          </button>
          {tab === "register" && <p>Rule: username min 3 chars, password min 4 chars.</p>}
          {error && <p className="error">{error}</p>}
        </div>
      )}

      {user && (
        <>
          <div className="card">
            <div className="row">
              <span className="chip">Player: {user}</span>
              <span className="chip">Score: {score}</span>
              <span className="chip">High Score: {highScore}</span>
              <span className="chip">Level: {level}</span>
              <span className="chip">Bonus Timer: {bonus ? bonusTimer : "-"}</span>
            </div>
            <div className="row" style={{ marginTop: 10 }}>
              <label>
                Mode{" "}
                <select value={mode} onChange={(e) => setMode(e.target.value)} disabled={running}>
                  <option value="classic">Classic</option>
                  <option value="wrap">Wrap</option>
                </select>
              </label>
              <label>
                Difficulty{" "}
                <select value={difficulty} onChange={(e) => setDifficulty(e.target.value)} disabled={running}>
                  <option value="easy">Easy</option>
                  <option value="normal">Normal</option>
                  <option value="hard">Hard</option>
                </select>
              </label>
              {!running ? <button onClick={() => onAnyStart()}>Start</button> : <button onClick={() => setPaused((v) => !v)}>{paused ? "Resume" : "Pause"}</button>}
              <button onClick={() => resetGame()}>Reset</button>
              <button onClick={() => { setUser(null); setRunning(false); }}>Logout</button>
            </div>
            <p className="start-hint">
              {gameOver ? "Game Over. Press any key or tap board to restart." : awaitingStart ? "Press any key or tap board to start." : "Controls: WASD / Arrow Keys / Touch buttons."}
            </p>
          </div>

          <div className="card game-stage" onClick={onAnyStart}>
            <canvas ref={canvasRef} width={GRID * CELL} height={GRID * CELL} />
          </div>

          <div className="card touch-controls">
            <h3>Touch Controls</h3>
            <div className="touch-row">
              <button className="touch-btn" onClick={() => onDirectionPress(DIR.UP)}>▲</button>
            </div>
            <div className="touch-row">
              <button className="touch-btn" onClick={() => onDirectionPress(DIR.LEFT)}>◀</button>
              <button className="touch-btn" onClick={() => onDirectionPress(DIR.DOWN)}>▼</button>
              <button className="touch-btn" onClick={() => onDirectionPress(DIR.RIGHT)}>▶</button>
            </div>
          </div>
        </>
      )}

      <div className="card">
        <div className="row">
          <h3 style={{ margin: 0 }}>Leaderboard</h3>
          <button onClick={clearLeaderboardScores}>Clear Scores</button>
          <button onClick={clearAllUsers}>Clear All Users</button>
        </div>
        <table>
          <thead>
            <tr>
              <th>#</th>
              <th>User</th>
              <th>High Score</th>
            </tr>
          </thead>
          <tbody>
            {leaderboard.map((u, i) => (
              <tr key={u.username}>
                <td>{i + 1}</td>
                <td>{u.username}</td>
                <td>{u.highScore}</td>
              </tr>
            ))}
            {leaderboard.length === 0 && (
              <tr>
                <td colSpan="3">No players yet.</td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);

