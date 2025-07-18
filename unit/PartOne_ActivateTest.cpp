#include "TestUtils.hpp"
#include "script_storages/DirectoryScriptStorage.h"

#include "Messages.h"

using Catch::Matchers::ContainsSubstring;

extern espm::Loader& GetEspmLoader();

class FakeDamageFormula : public IDamageFormula
{
public:
  [[nodiscard]] float CalculateDamage(const MpActor&, const MpActor&,
                                      const HitData&) const override
  {
    return 25.f;
  }

  [[nodiscard]] float CalculateDamage(const MpActor&, const MpActor&,
                                      const SpellCastData&) const override
  {
    return 25.f;
  }
};

// Actually, there are a few utils
espm::CompressedFieldsCache g_dummyCache;
PartOne& GetPartOne()
{
  auto instance = std::make_shared<PartOne>();
  instance->SetDamageFormula(std::make_unique<FakeDamageFormula>());

  instance->worldState.AttachScriptStorage(
    std::make_shared<DirectoryScriptStorage>(TEST_PEX_DIR));
  instance->AttachEspm(&GetEspmLoader());

  instance->worldState.bannedEspmCharacterRaceIds.clear();

  static std::vector<std::shared_ptr<PartOne>> g_partOneInstances;
  g_partOneInstances.push_back(instance);
  return *g_partOneInstances.back();
}

constexpr auto barrelInWhiterun = 0x4cc2d;

TEST_CASE("Activate without espm attached", "[PartOne][espm]")
{
  PartOne partOne;
  DoConnect(partOne, 0);

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::Activate },
                              { "data",
                                { { "caster", 0x15 },
                                  { "target", 0 },
                                  { "isSecondActivation", false } } } }),
    ContainsSubstring("No loaded esm or esp files are found"));
}

TEST_CASE("Activate without Actor attached", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::Activate },
                              { "data",
                                { { "caster", 0x15 },
                                  { "target", 0 },
                                  { "isSecondActivation", false } } } }),
    ContainsSubstring("Can't do this without Actor attached"));

  DoDisconnect(partOne, 0);
}

TEST_CASE("Activate with bad hoster", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::Activate },
                              { "data",
                                { { "caster", 0x15 },
                                  { "target", 0 },
                                  { "isSecondActivation", false } } } }),
    ContainsSubstring(
      "Bad hoster is attached to caster 0x15, expected 0xff000000, but "
      "found 0x0"));

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activate with incorrect WorldSpace", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::Activate },
                              { "data",
                                { { "caster", 0x14 },
                                  { "target", barrelInWhiterun },
                                  { "isSecondActivation", false } } } }),
    ContainsSubstring(
      "WorldSpace doesn't match: caster is in Tamriel (0x3c), target "
      "is in WhiterunWorld (0x1a26f)"));

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activation of an unexisting ref doesn't throw", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", 0xdeadbeef },
                                { "isSecondActivation", false } } } });

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("See harvested PurpleMountainFlower in Whiterun", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  partOne.Messages().clear();

  const auto refrId = 0x0100122a;
  auto& refr = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  refr.SetHarvested(true);

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  auto res = FindRefrMessage<CreateActorMessage>(partOne, refrId);
  REQUIRE(res.filteredMessages.size() == 1);
  REQUIRE(res.filteredMessages[0].props.isHarvested.has_value());
  REQUIRE(*res.filteredMessages[0].props.isHarvested == true);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
  refr.SetHarvested(false);
}

TEST_CASE("See open DisplayCaseSmFlat01 in Whiterun", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  partOne.Messages().clear();

  const auto refrId = 0x72080;
  auto& refr = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  refr.SetOpen(true);

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 25217.0293, -7373.9536, -3317.6880 }, 0,
                      0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  auto res = FindRefrMessage<CreateActorMessage>(partOne, refrId);
  REQUIRE(res.filteredMessages.size() == 1);
  REQUIRE(res.filteredMessages[0].props.isOpen.has_value());
  REQUIRE(*res.filteredMessages[0].props.isOpen == true);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
  refr.SetOpen(false);
}

TEST_CASE("Activate DisplayCaseSmFlat01 in Whiterun", "[PartOne][espm]")
{

  auto& partOne = GetPartOne();

  partOne.Messages().clear();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 25217.0293, -7373.9536, -3317.6880 }, 0,
                      0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  const auto refrId = 0x72080;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  auto res = FindRefrMessage<CreateActorMessage>(partOne, refrId);
  REQUIRE(res.filteredMessages.size() == 1);

  partOne.Messages().clear();

  REQUIRE(!ref.IsOpen());
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });
  REQUIRE(ref.IsOpen());
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });
  REQUIRE(!ref.IsOpen());

  REQUIRE(partOne.Messages().size() == 2);
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "true");
  REQUIRE(partOne.Messages()[0].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages()[0].j["propName"] == "isOpen");
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[1].j["dataDump"] == "false");
  REQUIRE(partOne.Messages()[1].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages()[1].j["propName"] == "isOpen");
  REQUIRE(partOne.Messages()[1].j["t"] == MsgType::UpdateProperty);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activate WRDoorMainGate01 in Whiterun", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 19367.3379, -7433.0698, -3547.4492 }, 0,
                      0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  partOne.Messages().clear();
  auto refrId = 0x1b1f3;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  ref.SetRelootTime(std::chrono::milliseconds(30));
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });
  REQUIRE(partOne.Messages().size() >= 1);
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "true");
  REQUIRE(partOne.Messages()[0].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages()[0].j["propName"] == "isOpen");
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);

  REQUIRE(partOne.Messages().size() >= 2);
  REQUIRE(partOne.Messages()[1].j["t"] == MsgType::Teleport);
  REQUIRE(partOne.Messages()[1].j["pos"].dump() ==
          nlohmann::json{ 19243.53515625, -7427.3427734375, -3595.4052734375 }
            .dump());
  REQUIRE(partOne.Messages()[1].j["rot"].dump() ==
          nlohmann::json{ 0.0, -0.0, -89.99922180175781 }.dump());
  REQUIRE(partOne.Messages()[1].j["worldOrCell"] == 0x3c);

  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  REQUIRE(ac.GetCellOrWorld() == FormDesc::Tamriel());

  partOne.Messages().clear();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  partOne.Tick();

  REQUIRE(ref.IsOpen() == false);

  REQUIRE((ac.GetPos() -
           NiPoint3(19243.53515625, -7427.3427734375, -3595.4052734375))
            .Length() < 5);
  REQUIRE(ac.GetAngle() == NiPoint3{ 0.0, -0.0, -89.99922180175781 });

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activate PurpleMountainFlower in Whiterun", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();

  partOne.Messages().clear();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f);
  partOne.SetUserActor(0, 0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  ac.RemoveAllItems();

  const auto refrId = 0x0100122a;
  const auto MountainFlower01Purple = 0x77e1e;

  auto res = FindRefrMessage<CreateActorMessage>(partOne, refrId);
  REQUIRE(res.filteredMessages.size() == 1);

  partOne.Messages().clear();

  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  ref.SetRelootTime(std::chrono::milliseconds(25));

  REQUIRE(!ref.IsHarvested());

  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });

  partOne.Tick(); // send deferred inventory update messages

  REQUIRE(partOne.Messages().size() >= 2);
  REQUIRE(partOne.Messages()[0].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "true");
  REQUIRE(partOne.Messages()[0].j["propName"] == "isHarvested");
  REQUIRE(partOne.Messages()[1].j["t"] == MsgType::SetInventory);
  REQUIRE(partOne.Messages()[1].j["inventory"].dump() ==
          nlohmann::json({ { "entries",
                             { { { "baseId", MountainFlower01Purple },
                                 { "count", 1 } } } } })
            .dump());

  REQUIRE(ref.IsHarvested());

  // We should not be able to harvest already harvested items
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);
  ref.Activate(ac);
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);

  partOne.Tick();
  REQUIRE(ref.IsHarvested());

  partOne.Messages().clear();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  partOne.Tick();
  REQUIRE(!ref.IsHarvested());
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "false");
  REQUIRE(partOne.Messages()[0].j["propName"] == "isHarvested");

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("BarrelFood01 PutItem/TakeItem", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  auto refrId = 0x20570;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  ref.SetRelootTime(std::chrono::milliseconds(25));

  REQUIRE(ref.GetInventory().IsEmpty());

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 21272.0000, -7816.0000, -3608.0000 }, 0,
                      0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  actor.RemoveAllItems();

  REQUIRE_THROWS_WITH(
    ref.PutItem(actor, { 0x12eb7, 2 }),
    ContainsSubstring("Actor 0xff000000 doesn't occupy ref 0x20570"));

  // Activation forces base container to be added
  partOne.Messages().clear();
  REQUIRE(ref.GetInventory().GetTotalItemCount() == 0);
  ref.SetChanceNoneOverride(0); // LeveledItems will never produce zero items
  ref.Activate(actor);
  REQUIRE(ref.GetInventory().GetTotalItemCount() > 0);

  REQUIRE(partOne.Messages().size() >= 1);
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[0].j["propName"] == "isOpen");
  REQUIRE(partOne.Messages()[0].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "true");
  REQUIRE(partOne.Messages().size() >= 2);
  REQUIRE(partOne.Messages()[1].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[1].j["propName"] == "inventory");
  REQUIRE(partOne.Messages()[1].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages().size() == 3);
  REQUIRE(partOne.Messages()[2].j["t"] == MsgType::OpenContainer);
  REQUIRE(partOne.Messages()[2].j["target"] == ref.GetFormId());

  REQUIRE_THROWS_WITH(
    ref.PutItem(actor, { 0x12eb7, 2 }),
    ContainsSubstring("Source inventory doesn't have enough 0x12eb7 "
                      "(2 is required while 0 present)"));

  actor.AddItem(0x12eb7, 1);
  REQUIRE_THROWS_WITH(
    ref.PutItem(actor, { 0x12eb7, 2 }),
    ContainsSubstring("Source inventory doesn't have enough 0x12eb7 "
                      "(2 is required while 1 present)"));

  actor.AddItem(0x12eb7, 1);

  // On the first PutItem, we use DoMessage to ensure our messages are parsed
  // and processed successfully.
  // "Worn" state may be sent by the real skymp client. Ignoring then.
  partOne.Messages().clear();
  DoMessage(partOne, 0,
            { { "t", MsgType::PutItem },
              { "baseId", 0x12eb7 },
              { "count", 2 },
              { "worn", true },
              { "target", refrId } });
  partOne.Tick(); // send deferred inventory update messages
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::SetInventory);

  REQUIRE(actor.GetInventory().IsEmpty());
  REQUIRE(ref.GetInventory().GetItemCount(0x12eb7) == 2);

  // Keeps items from base container
  auto copy = ref.GetInventory();
  copy.RemoveItems({ { 0x12eb7, 2 } });
  REQUIRE(copy.GetTotalItemCount() > 0);

  partOne.Messages().clear();
  DoMessage(partOne, 0,
            { { "t", MsgType::TakeItem },
              { "baseId", 0x12eb7 },
              { "count", 1 },
              { "target", refrId } });
  partOne.Tick(); // send deferred inventory update messages
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::SetInventory);
  REQUIRE(ref.GetInventory().GetItemCount(0x12eb7) == 1);
  REQUIRE(actor.GetInventory().GetItemCount(0x12eb7) == 1);

  // Reloot check
  {
    // Take all items from the barrel. This action forces reloot.
    for (auto e : ref.GetInventory().entries)
      ref.TakeItem(actor, e);

    REQUIRE(ref.GetInventory().IsEmpty() == true);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    partOne.Tick();

    REQUIRE(ref.GetInventory().IsEmpty() == false);
  }

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Server creates and destroys an object for user correcly",
          "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  partOne.Messages().clear();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 16230, -8377, -4564 }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);

  auto refId = 0x01000f69;

  auto res = FindRefrMessage<CreateActorMessage>(partOne, refId);
  REQUIRE(res.filteredMessages.size() == 1);

  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000ABC);
  ac.SetPos({ 0, 0, 0 });

  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refId);

  auto res2 = FindRefrMessageIdx<DestroyActorMessage>(partOne, ref.GetIdx());
  REQUIRE(res2.filteredMessages.size() == 1);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000ABC);
}

TEST_CASE("Activate BarrelFood01 in Whiterun (open/close)", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 21272.0000, -7816.0000, -3608.0000 }, 0,
                      0x1a26f);
  partOne.SetUserActor(0, 0xff000000);

  auto refrId = 0x20570;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  // Open/close container
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });
  REQUIRE(ref.IsOpen());
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", true } } } });
  REQUIRE(!ref.IsOpen());

  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });
  REQUIRE(ref.IsOpen());

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);

  // Actor destruction forces contaner close
  REQUIRE(!ref.IsOpen());
}

TEST_CASE("Activate torch", "[espm][PartOne]")
{
  auto& partOne = GetPartOne();
  partOne.Messages().clear();
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 6507.0527, -5283.5664, -3263.1714 }, 0,
                      0x56C1B);
  partOne.SetUserActor(0, 0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  ac.RemoveAllItems();

  const auto refrId = 0x671a8;
  const auto torchBaseId = 0x1d4ec;

  auto res = FindRefrMessage<CreateActorMessage>(partOne, refrId);
  REQUIRE(res.filteredMessages.size() == 1);

  partOne.Messages().clear();

  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  ref.SetRelootTime(std::chrono::milliseconds(25));

  REQUIRE(!ref.IsHarvested());

  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::Activate },
                            { "data",
                              { { "caster", 0x14 },
                                { "target", refrId },
                                { "isSecondActivation", false } } } });

  partOne.Tick(); // send deferred inventory update messages

  REQUIRE(partOne.Messages().size() >= 2);
  REQUIRE(partOne.Messages()[0].j["idx"] == ref.GetIdx());
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "true");
  REQUIRE(partOne.Messages()[0].j["propName"] == "isHarvested");
  REQUIRE(partOne.Messages()[1].j["t"] == MsgType::SetInventory);
  REQUIRE(
    partOne.Messages()[1].j["inventory"].dump() ==
    nlohmann::json(
      { { "entries", { { { "baseId", torchBaseId }, { "count", 1 } } } } })
      .dump());

  REQUIRE(ref.IsHarvested());

  // We should not be able to harvest already harvested items
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);
  ref.Activate(ac);
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);

  partOne.Tick();
  REQUIRE(ref.IsHarvested());

  partOne.Messages().clear();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  partOne.Tick();
  REQUIRE(!ref.IsHarvested());
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(partOne.Messages()[0].j["dataDump"] == "false");
  REQUIRE(partOne.Messages()[0].j["propName"] == "isHarvested");

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Regress test for 'Record ff00b5de doesn't exist'",
          "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  partOne.Messages().clear();

  const auto refrId = 0x000524f2;

  // 1: C:\skymp\unit\PartOne_ActivateTest.cpp(598): FAILED:
  // 1: due to unexpected exception with message:
  // 1:   Record ff00b5de doesn't exist

  auto& actor = partOne.worldState.GetFormAt<MpActor>(refrId);

  // REQUIRE(actor.GetInventory().ToJson().dump() == "");

  // TODO: Why 0???
  // REQUIRE(actor.GetInventory().GetItemCount(0x0000B5DE) == 1);
}

TEST_CASE("Regress: LvlGiant mustn't have Fox race health", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  partOne.worldState.npcEnabled = true;
  partOne.Messages().clear();

  const auto refrId = 0x000ecf8a;

  auto& actor = partOne.worldState.GetFormAt<MpActor>(refrId);

  uint32_t baseId = actor.GetBaseId();
  uint32_t raceId = actor.GetRaceId();

  auto baseActorValues = GetBaseActorValues(&partOne.worldState, baseId,
                                            raceId, actor.GetTemplateChain());

  REQUIRE(baseActorValues.health == 250.f);
}
