import asyncio
import json
import logging
import random
import string
import time
from contextlib import asynccontextmanager

import psycopg
import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from serial import Serial, SerialException

from app.routers import planes

logger = logging.getLogger("uvicorn.error")

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

PORT = "/dev/ttyACM0"
BAUD = 115200
POLL_INTERVAL = 1.0

conn_info = "dbname=wheat_weywot user=postgres password=... host=localhost"

# ---------------------------------------------------------------------------
# Models
# ---------------------------------------------------------------------------

class Plane(BaseModel):
    flightName: str
    long: float
    lat: float




# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",
        "http://localhost:5173",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(planes.router)

@app.get("/health")
def get_health():
    return {200: "ok"}

# ---------------------------------------------------------------------------

if __name__ == "__main__":
    uvicorn.run(
        "main:app",
        host="localhost",
        port=8000,
        reload=True,
    )