# Plane Tracker — Setup & Run

## Prerequisites

- Python 3.12+
- Node.js 18+
- PostgreSQL running locally
- nRF52840 board flashed and connected via USB

---

## 1. Database

```bash
psql -U postgres -h localhost
```

```sql
CREATE DATABASE wheat_weywot;
\c wheat_weywot
CREATE TABLE flights (
    id SERIAL PRIMARY KEY,
    flightname VARCHAR(16),
    long FLOAT,
    lat FLOAT
);
\q
```

---

## 2. Backend

```bash
cd dashboard/backend
python3 -m venv .venv
source .venv/bin/activate
pip install fastapi uvicorn psycopg pyserial h3
```

Update the DB password in `planes.py` and `main.py`:
```python
conn_info = "dbname=wheat_weywot user=postgres password=YOUR_PASSWORD host=localhost"
```

Run:
```bash
uvicorn main:app --host localhost --port 8000 --reload
```

The backend will:
- Seed 30 dummy planes into the DB on startup
- Attempt to connect to the board on `/dev/ttyACM0`
- Poll `plane print` every second and update `/planes/list` live

If the board isn't connected, dummy data is still served from memory.

---

## 3. Frontend

```bash
cd dashboard/frontend
npm install
npm run dev
```

Runs on `http://localhost:5173`.

---

## 4. Board

Flash the firmware and connect via USB. The backend picks it up automatically — no manual serial script needed.

To test the board manually:
```bash
# Add a plane
plane add QFA123 153.0251 -27.4698 153.0100 -27.4600

# Print heap as JSON
plane print
```

---

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/planes/list` | Current planes (live from serial or dummy) |
| GET | `/planes/coords` | Convex hull polygons for map overlay |
| GET | `/health` | Health check |
