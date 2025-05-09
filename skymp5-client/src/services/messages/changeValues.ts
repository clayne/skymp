import { MsgType } from "../../messages";
import { ActorValues } from "../../sync/actorvalues";

export interface ChangeValuesMessage {
    t: MsgType.ChangeValues;
    data: Partial<ActorValues>;
    idx: number;
}
