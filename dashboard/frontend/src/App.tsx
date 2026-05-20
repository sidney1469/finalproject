import { useState, useEffect } from "react";
import getPlanes, { getCoords } from "./webhooks/planes";
import "./App.css";
import MapRender from "./webhooks/map";
import Heading from "./component/heading";
import { getHealth } from "./webhooks/health";
import SidePanel from "./component/sidepanel";
import type { Flight } from "./component/sidepanel/flight";

type Coords = {
  long: number;
  lat: number;
};

type DensityPolygon = Coords[];

function App() {
  const [flights, setFlight] = useState<Flight[] | null>(null);
  const [selectedFlight, setSelectedFlight] = useState<[number, number] | null>(
    null
  );

  const [polys, setPoly] = useState<DensityPolygon[] | null>(null);

  useEffect(() => {
    const updatePlanes = async () => {
      try {
        setFlight(await getPlanes());
      } catch (err) {
        console.log("failed to get planes");
      }
    };

    const updateCoords = async () => {
      try {
        setPoly(await getCoords());
      } catch (err) {
        console.log("failed to get polygons");
      }
    };

    updatePlanes();
    updateCoords();

    const planeInterval = setInterval(updatePlanes, 1000);
    const coordsInterval = setInterval(updateCoords, 5000);

    return () => {
      clearInterval(planeInterval);
      clearInterval(coordsInterval);
    };
  }, []);

  useEffect(() => {
    const fetchUsers = async () => {
      try {
        await getHealth();
      } catch (err) {
        console.log("Failed to load users");
      }
    };

    fetchUsers();
  }, []);

  return (
    <>
      <Heading />

      <div className="panels">
        <div className="main-panel">
          <MapRender
            flights={flights}
            selectedFlight={selectedFlight}
            list={polys}
          />
        </div>

        <div className="side-panel">
          <SidePanel flights={flights} setSelectedFlight={setSelectedFlight} />
        </div>
      </div>
    </>
  );
}

export default App;