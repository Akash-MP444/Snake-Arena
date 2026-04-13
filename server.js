const express = require("express");
const fs = require("fs/promises");
const path = require("path");
const crypto = require("crypto");

const app = express();
const PORT = process.env.PORT || 3000;
const DATA_DIR = path.join(__dirname, "data");
const USERS_FILE = path.join(DATA_DIR, "users.json");

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

function hashPassword(password) {
  return crypto.createHash("sha256").update(password).digest("hex");
}

async function ensureDataFile() {
  await fs.mkdir(DATA_DIR, { recursive: true });
  try {
    await fs.access(USERS_FILE);
  } catch {
    await fs.writeFile(USERS_FILE, "[]", "utf8");
  }
}

async function readUsers() {
  await ensureDataFile();
  const raw = await fs.readFile(USERS_FILE, "utf8");
  const parsed = JSON.parse(raw);
  return Array.isArray(parsed) ? parsed : [];
}

async function writeUsers(users) {
  await fs.writeFile(USERS_FILE, JSON.stringify(users, null, 2), "utf8");
}

app.get("/api/health", (_req, res) => {
  res.json({ ok: true });
});

app.post("/api/register", async (req, res) => {
  const { username, password } = req.body || {};
  if (
    typeof username !== "string" ||
    typeof password !== "string" ||
    username.trim().length < 3 ||
    password.length < 4
  ) {
    return res.status(400).json({
      error: "Username must be at least 3 characters and password at least 4 characters."
    });
  }

  const cleanUser = username.trim();
  const users = await readUsers();
  const exists = users.some((u) => u.username.toLowerCase() === cleanUser.toLowerCase());
  if (exists) {
    return res.status(409).json({ error: "Username already exists." });
  }

  users.push({
    username: cleanUser,
    passwordHash: hashPassword(password),
    highScore: 0
  });
  await writeUsers(users);
  res.json({ ok: true, username: cleanUser, highScore: 0 });
});

app.post("/api/login", async (req, res) => {
  const { username, password } = req.body || {};
  if (typeof username !== "string" || typeof password !== "string") {
    return res.status(400).json({ error: "Invalid credentials." });
  }

  const users = await readUsers();
  const user = users.find((u) => u.username.toLowerCase() === username.trim().toLowerCase());
  if (!user || user.passwordHash !== hashPassword(password)) {
    return res.status(401).json({ error: "Invalid credentials." });
  }
  res.json({ ok: true, username: user.username, highScore: user.highScore || 0 });
});

app.post("/api/score", async (req, res) => {
  const { username, score } = req.body || {};
  if (typeof username !== "string" || typeof score !== "number" || score < 0) {
    return res.status(400).json({ error: "Invalid score payload." });
  }

  const users = await readUsers();
  const idx = users.findIndex((u) => u.username.toLowerCase() === username.trim().toLowerCase());
  if (idx < 0) {
    return res.status(404).json({ error: "User not found." });
  }

  if (score > (users[idx].highScore || 0)) {
    users[idx].highScore = Math.floor(score);
    await writeUsers(users);
  }

  res.json({ ok: true, highScore: users[idx].highScore || 0 });
});

app.get("/api/leaderboard", async (_req, res) => {
  const users = await readUsers();
  const top = users
    .map((u) => ({ username: u.username, highScore: u.highScore || 0 }))
    .sort((a, b) => b.highScore - a.highScore)
    .slice(0, 10);
  res.json(top);
});

app.post("/api/leaderboard/clear", async (_req, res) => {
  const users = await readUsers();
  await writeUsers([]);
  res.json({ ok: true, cleared: users.length });
});

app.post("/api/leaderboard/clear-scores", async (_req, res) => {
  const users = await readUsers();
  const resetUsers = users.map((u) => ({ ...u, highScore: 0 }));
  await writeUsers(resetUsers);
  res.json({ ok: true, cleared: resetUsers.length });
});

app.listen(PORT, async () => {
  await ensureDataFile();
  console.log(`Snake web app running: http://localhost:${PORT}`);
});

