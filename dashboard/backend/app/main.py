from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routers import planes
import uvicorn

app = FastAPI()

app.include_router(planes.router)

@app.get("/health")
def get_health():
    return {200: "ok"}

origins = [
    "http://localhost:3000",  # Common for React
    "http://localhost:5173",  # Common for Vue/LiveServer
    "https://yourdomain.com", # Production domain
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,            # Allows specific origins
    allow_credentials=True,           # Allows cookies and auth headers
    allow_methods=["*"],              # Allows all HTTP methods (GET, POST, etc.)
    allow_headers=["*"],              # Allows all headers
)

if __name__ == '__main__':
    uvicorn.run(
            "main:app",  # Pass as a string to enable reload
            host="localhost",
            port=8000,
            reload=True,
        )