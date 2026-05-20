import "./styles.css";
import FlightList from "./sidepanel/flightlist";
import type { Flight } from "./sidepanel/flight";
import type { Dispatch, SetStateAction } from "react";

type SidePanelProps = {
  flights?: Flight[] | null;
  setSelectedFlight: Dispatch<SetStateAction<[number, number] | null>>;
};

const SidePanel = ({ flights, setSelectedFlight }: SidePanelProps) => {
  return (
    <>
      <div style={{ display: "flex", flex: 1, flexDirection: "row" }}>
        <h2 className="sidepanelh1">Flight</h2>
        <h2 className="sidepanelh1">Long</h2>
        <h2 className="sidepanelh1">Lat</h2>
      </div>

      <FlightList
        flights={flights}
        setSelectedFlight={setSelectedFlight}
      />

      <button className="unselectButton" onClick={() => setSelectedFlight(null)}>
        Unselect Plane
      </button>
    </>
  );
};

export default SidePanel;