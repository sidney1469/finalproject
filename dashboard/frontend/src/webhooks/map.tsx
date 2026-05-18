import "leaflet/dist/leaflet.css";
import { useEffect, useState } from "react";
import {
  MapContainer,
  TileLayer,
  Marker,
  Popup,
  useMap,
  Polyline,
  Polygon,
} from "react-leaflet";
import { FaPlaneUp } from "react-icons/fa6";
import { renderToStaticMarkup } from "react-dom/server";
import L from "leaflet";
import type { Flight } from "../component/sidepanel/flight";
import "./styles.css";

const planeIcon = L.divIcon({
  html: renderToStaticMarkup(<FaPlaneUp size={28} color="black" />),
  className: "plane-marker",
  iconSize: [28, 28],
  iconAnchor: [14, 14],
});

const selectedPlaneIcon = L.divIcon({
  html: renderToStaticMarkup(<FaPlaneUp size={38} color="white" />),
  className: "selected-plane-marker",
  iconSize: [38, 38],
  iconAnchor: [19, 19],
});

type Coords = {
  long: number;
  lat: number;
};

type DensityPolygon = Coords[];

type MapRenderProps = {
  flights?: Flight[] | null;
  selectedFlight?: [number, number] | null;
  list?: DensityPolygon[] | null;
};

type FlightHistoryPoint = {
  position: [number, number];
  timestamp: number;
};

type FlightHistory = Record<string, FlightHistoryPoint[]>;

const defaultCenter: [number, number] = [-27.4705, 153.026];

const isSamePosition = (
  flight: Flight,
  selectedFlight?: [number, number] | null
) => {
  if (!selectedFlight) return false;

  const [selectedLat, selectedLong] = selectedFlight;

  return (
    Math.abs(flight.lat - selectedLat) < 0.000001 &&
    Math.abs(flight.long - selectedLong) < 0.000001
  );
};

const getPolygonArea = (polygon: DensityPolygon): number => {
  if (polygon.length < 3) return 0;

  let area = 0;

  for (let i = 0; i < polygon.length; i++) {
    const current = polygon[i];
    const next = polygon[(i + 1) % polygon.length];

    area += current.long * next.lat - next.long * current.lat;
  }

  return Math.abs(area / 2);
};

const getPolygonColour = (area: number, maxArea: number): string => {
  if (maxArea === 0) return "lime";

  const sizeRatio = area / maxArea;

  if (sizeRatio > 0.75) return "red";
  if (sizeRatio > 0.5) return "orange";
  if (sizeRatio < 0.5) return "yellow";

  return "lime";
};

const MoveToSelectedFlight = ({
  selectedFlight,
}: {
  selectedFlight?: [number, number] | null;
}) => {
  const map = useMap();

  useEffect(() => {
    if (!selectedFlight) return;

    map.invalidateSize();
    map.flyTo(selectedFlight, 9);
  }, [selectedFlight, map]);

  return null;
};

const MapRender = ({ flights, selectedFlight, list }: MapRenderProps) => {
  const [selectedFlightName, setSelectedFlightName] = useState<string | null>(
    null
  );

  const [flightHistory, setFlightHistory] = useState<FlightHistory>({});

  useEffect(() => {
    if (!selectedFlight) {
      setSelectedFlightName(null);
      return;
    }

    if (!flights) return;

    const clickedFlight = flights.find((flight) =>
      isSamePosition(flight, selectedFlight)
    );

    if (clickedFlight) {
      setSelectedFlightName(clickedFlight.flightName);
    }
  }, [selectedFlight, flights]);

  useEffect(() => {
    if (!flights) return;

    const now = Date.now();
    const sixtySecondsAgo = now - 60_000;

    setFlightHistory((previousHistory) => {
      const newHistory: FlightHistory = { ...previousHistory };

      flights.forEach((flight) => {
        const oldPoints = newHistory[flight.flightName] ?? [];

        const newPoint: FlightHistoryPoint = {
          position: [flight.lat, flight.long],
          timestamp: now,
        };

        newHistory[flight.flightName] = [...oldPoints, newPoint].filter(
          (point) => point.timestamp >= sixtySecondsAgo
        );
      });

      return newHistory;
    });
  }, [flights]);

  const selectedPlane = flights?.find(
    (flight) => flight.flightName === selectedFlightName
  );

  const selectedPlanePosition: [number, number] | null =
    selectedFlightName && selectedPlane
      ? [selectedPlane.lat, selectedPlane.long]
      : null;

  const selectedPlaneTrail =
    selectedFlightName && flightHistory[selectedFlightName]
      ? flightHistory[selectedFlightName].map((point) => point.position)
      : [];

  const polygonAreas = list?.map((polygon) => getPolygonArea(polygon)) ?? [];

  const maxPolygonArea =
    polygonAreas.length > 0 ? Math.max(...polygonAreas) : 0;

  return (
    <MapContainer
      center={defaultCenter}
      zoom={10}
      style={{ height: "500px", width: "100%" }}
    >
      <MoveToSelectedFlight selectedFlight={selectedPlanePosition} />

      <TileLayer
        attribution='&copy; OpenStreetMap contributors'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />

      {list?.map((polygon, index) => {
        if (polygon.length < 3) return null;

        const positions: [number, number][] = polygon.map((coord) => [
          coord.lat,
          coord.long,
        ]);

        const area = polygonAreas[index] ?? 0;
        const colour = getPolygonColour(area, maxPolygonArea);

        return (
          <Polygon
            key={`density-polygon-${index}`}
            positions={positions}
            pathOptions={{
              color: colour,
              fillColor: colour,
              fillOpacity: 0.25,
              weight: 2,
            }}
          >
            <Popup>
              <strong>Density Area</strong>
              <br />
              Size Rank: {(area / maxPolygonArea).toFixed(2)}
              <br />
              Points: {polygon.length}
            </Popup>
          </Polygon>
        );
      })}

      {selectedPlaneTrail.length > 1 && (
        <Polyline
          positions={selectedPlaneTrail}
          pathOptions={{
            color: "black",
            weight: 4,
            opacity: 0.8,
          }}
        />
      )}

      {flights?.map((flight) => {
        const selected = flight.flightName === selectedFlightName;

        return (
          <Marker
            key={flight.flightName}
            position={[flight.lat, flight.long]}
            icon={selected ? selectedPlaneIcon : planeIcon}
            zIndexOffset={selected ? 1000 : 0}
          >
            <Popup>
              <strong>{flight.flightName}</strong>
              <br />
              Lat: {flight.lat.toFixed(3)}
              <br />
              Long: {flight.long.toFixed(3)}
            </Popup>
          </Marker>
        );
      })}
    </MapContainer>
  );
};

export default MapRender;