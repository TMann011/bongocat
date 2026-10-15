# Development Log & Technical Analysis: Bongo Cat Internal Architecture

**Date**: 2026-10-02  
**Target Runtime**: Unity 6 (MonoBleedingEdge x64)  
**Binary Target**: `BongoCat.exe` (Steam AppID `3419430`)  
**Scope**: Paw Pass mechanics, input handling subsystem, inventory resolution, and runtime architecture analysis.

---

## 1. Executive Summary

This document consolidates reverse engineering findings, engine architecture investigations, and runtime behavior observations from the Halloween Edition update (`02_HalloweenEdition`, Season ID `2610`). The analysis focused on four core subsystems:
1. **Pass Progress & Milestone Computation** (`PawPassManager`, `PawPassEntity`, `StatsInitializer`).
2. **Item Ownership & Inventory Resolution** (`SteamItemRuntime`, `SteamItemBase`, `ItemData`).
3. **Global Input Subsystem & Keystroke Processing** (`GlobalKeyHook`, `WinKeyHook`).
4. **Shop State & Refresh Timers** (`Shop`, `Chest_Token`).

---

## 2. Pass Progress Subsystem (`PawPassManager`)

### 2.1 Call Flow & State Validation
Decompilation of `BongoCat.PawPass.PawPassManager` revealed why progress calculations previously failed or returned zero:

```csharp
public static int TapsFor(PawPassEntity pass)
{
    if (!StatsInitializer.Initialized || !pass.IsJoined)
        return 0;
    return Mathf.Clamp(pass.ProgressStat.IntValue(), 0, pass.TotalTaps);
}
```

### 2.2 Gatekeeping Conditions
- **`StatsInitializer.Initialized`**: Requires local Steam client user stats callback (`UserStatsReceived_t`) to successfully complete before stat integers are read.
- **`pass.IsJoined`**:
  - Defined as `HasFreePass || HasPremiumPass`.
  - Both properties check `FreeKey.HasItem` and `PremiumKey.HasItem`.
  - `HasItem` queries `ItemData.GetTotalQuantity() > 0`.
- **Root Cause of Zero-Progress Display**: If `FreeKey.HasItem` evaluated to `0`, `IsJoined` returned `false`. Consequently, `TapsFor` immediately returned `0` taps, regardless of the underlying stat value stored in Steam client memory.

### 2.3 Milestone Calculation Formula
Milestones are structured in tiers with linear tap thresholds (150,000 taps required for 30 milestones in Season `2610`). The progress gauge clamps between `0` and `pass.TotalTaps`.

---

## 3. Inventory & Ownership Check Architecture

### 3.1 JIT Compilation & Caller Sites
The Mono 6 JIT engine compiles item quantity evaluations into an optimized call sequence across 6 primary locations:
- `SteamItemRuntime.IsOwned`
- `SteamItemBase.HasItem`
- `PawPassEntity.HasFreePass`
- `PawPassEntity.HasPremiumPass`
- `DisplayedItemAmount`
- `Shop` item validation logic

### 3.2 Call-Site Assembly Pattern
All call sites adhere to the following disassembly structure:
```x86asm
41 FF D3              ; call r11 (points to ItemData.GetTotalQuantity)
48 85 C0              ; test rax, rax
40 0F 9F C0           ; setg al
48 0F B6 C0           ; movzx rax, al
```

Directly resolving `ItemData.GetTotalQuantity` or replacing caller test-and-set logic standardizes ownership queries across the UI without destabilizing the Mono garbage collector or object allocations.

---

## 4. Input Architecture & Keystroke Polling (`WinKeyHook`)

### 4.1 Implementation Mechanics
- `BongoCat.OSSpecific.GlobalKeyHook` utilizes a polling loop rather than a standard Win32 message queue (`GetMessage`/`DispatchMessage`).
- `WinKeyHook.cs` directly polls key states using:
  ```csharp
  GetAsyncKeyState(BUTTONS[i]) == -32768
  ```

### 4.2 Impact on Automation
- Standard `PostMessageW(hWnd, WM_KEYDOWN, VK_SPACE, ...)` posts into the thread message queue.
- Because `GetAsyncKeyState` queries the Win32 physical keyboard driver state table in kernel mode (`win32k.sys`), synthetic window messages do not update this table and are skipped by `WinKeyHook`.
- To register input with `GetAsyncKeyState`, synthetic hardware-level inputs (`keybd_event` or `SendInput`) are required.
- Keys such as `VK_F24` (virtual key code `0x87` / `135`) are defined in `WinKeys.cs` and can be leveraged for non-intrusive tap registration without interfering with foreground text fields.

---

## 5. Shop & Refresh Timers (`Shop`)

### 5.1 Token Verification
- `Shop.cs` checks the quantity of `Game.Inventory.Chest_Token`.
- When token quantity is `0`, a 60-second cooldown timer (`StockRefreshTimeLeft = 60`) is initiated.
- When token quantity is `> 0`, `StockRefreshTimeLeft` is cleared to `0` and `ChestIsReady` evaluates to `true`.

---

## 6. Summary of Architectural Changes & Revisions

| Component | Previous Mechanism | Updated Architecture |
|---|---|---|
| **Ownership Validation** | Single JIT hook on `SteamItemRuntime.IsOwned` | Universal handling across `ItemData.GetTotalQuantity` and call sites |
| **Pass Synchronization** | Pure stat write via Steam API | Stat injection coupled with pass eligibility verification (`IsJoined`) |
| **Input Generation** | Window message posting (`PostMessageW`) | Hardware event dispatch (`SendInput`/`keybd_event`) targeting mapped virtual keys |
| **Shop Timers** | Memory write to timer variables | Token state resolution enabling dynamic refresh evaluation |

---

## 7. Complete Implemented Feature Matrix (Trainer v3.0)

| Index | Feature | Mechanism | Target Subsystem |
|---|---|---|---|
| **[1]** | **RAM Skin & Pass Unlocker** | JIT Code Cave detour on `IsOwned` caller sites + `ItemData.GetTotalQuantity()` patched to return `999` (`48 C7 C0 E7 03 00 00 C3`) | `SteamItemRuntime`, `ItemData`, Mono JIT |
| **[2]** | **Unhide All Collections** | Clears `EXCLUDE_EVENT_ITEMS` and `ExcludeEventItems` in `HKCU\Software\Irox Games\BongoCat` | `BongoDex`, `HideUntilReceived` |
| **[3]** | **PawPass Lv99 Max** | Direct in-memory Steam UserStats remote thread injection (`PP_PR_2610=500k`, `Has_PPP_2610=1`) + baseline reset (`PP_BL_*=0`) | `PawPassManager`, `SteamAPI` |
| **[4]** | **Permanent Skin Save** | Prunes bloated keys (>300 bytes) and syncs equipped wardrobe selections to registry | `CatCosmetics`, `EQUIPPED_ITEMS_3` |
| **[5]** | **Battle Royale Match Trigger** | Injects future Unix timestamp (`now + 10s`) into `BattleCountdownEnd` | `BattleRoyale.cs`, Lobby HUD |
| **[6]** | **Item Reward Popup Trigger** | Enables `TRIGGER_ITEM_DROPS` & `ITEM_GENERATOR_ACTIVE` local popup generation | `NewItemPopup.cs`, UI Animation |
| **[7]** | **Cosmetic Auto-Randomizer** | Sets `HAT_RANDOMIZER=1` for native wardrobe rotation cycle | `CatInventory`, Wardrobe |
| **[8]** | **Multi-Cat Desktop Slots** | Configures `MultiplayerSpawnPattern=8` (Free) and unblocks desktop slot bounds | `CatSpawnManager`, Spawning |
| **[9]** | **Hardware Auto-Tapper** | Dispatches non-intrusive `keybd_event(VK_F24, ...)` at ~28 taps/sec; recognized by `WinKeyHook.cs` (`GetAsyncKeyState`) without foreground text pollution | `WinKeyHook.cs`, `GlobalKeyHook` |
| **[10]** | **Infinite Coins Stash** | Sets `BongoTap` and `BongoBeat` to `999,999,999` with economy boost multipliers | `Game.Stats`, Pets Economy |
| **[11]** | **Instant Chests & 999x Tokens** | Clears `TIME_LEFT`, `TIME_LEFT_e`, `EMOTE_CHEST_TIME_LEFT` to 0s continuously, sets 999 tokens via `GetTotalQuantity()` hook, maxes coin stash (999M) | `Shop.cs`, `ChestExchanger`, `Chest_Token` |
| **[12]** | **Steam Stats & Achievements Max** | Injects lifetime stats (`EmotesUsed=100k`, `MaxLobbySize=16`, `TotalItems=500`, `Exchanges=100`) via remote `ISteamUserStats` | `AchievementStats.cs`, Steamworks |
| **[13]** | **Desktop Click-Through Mode** | Toggles `WS_EX_TRANSPARENT \| WS_EX_LAYERED` on Unity window handle | `WinTransparentWindow.cs`, Win32 |
| **[14]** | **Self-Petting & Enhanced Reacts** | Sets `PatOwnCat=1`, `ShowPatEmote=1`, `ReceivePats=1`, `CatBobbing=1`, and `MeowMeow=1` | `CatPatter.cs`, `CatSpeech.cs` |
| **[15]** | **Instant BR Victory & Promo** | Forces `BattleHasWon=1` and triggers winner sequence granting exclusive crown cosmetic | `BattleRoyale.cs`, `PromoItemChecker` |

---

## 8. Deep Reverse Engineering: Multiplayer Skin Replication & Infinite Chest Claiming

### 8.1 Multiplayer Skin Visibility Verification (`CatCosmetics` & `CatCosmeticsMultiplayer`)
1. **The Vanilla Local-Only Trap**:
   In `CatCosmetics.EquipItem()`:
   ```csharp
   bool flag = steamItem.RuntimeData.GetTotalQuantity() == 0;
   Equip(steamItem, playAnimation, flag);
   ```
   When `flag == true` (unowned item):
   `Equip()` sets `isLocal = true` and ONLY mutates `_locallyEquippedItems`. It never touches `LobbyMemberData.Current.Me` and never updates `EQUIPPED_ITEMS_3`.
2. **The Multiplayer Replication Fix**:
   With our JIT patch on `ItemData.GetTotalQuantity()` returning `999` (`mov rax, 999; ret`), `flag` is always `false`!
   This causes `Equip()` to execute:
   ```csharp
   LobbyMemberData me = LobbyData.Current.Me;
   me[steamItem.ItemSlot] = steamItem.Id.ToString();
   EquippedItems = RemoveItemsWithSameCategory(EquippedItems, steamItem);
   PlayerPrefsHelp.SetObject("EQUIPPED_ITEMS_3", EquippedItems.Select(i => i.Id).ToArray());
   ```
   `me[steamItem.ItemSlot] = steamItem.Id.ToString()` directly calls `Matchmaking.Client.SetLobbyMemberData(lobby, "skin", id)` (and `"hat"`).
3. **Remote Client Rendering**:
   Remote clients hook `Events.OnLobbyDataUpdate`:
   ```csharp
   EquipCosmetic("hat", _lobbyMemberData.Data.Hat);
   EquipCosmetic("skin", _lobbyMemberData.Data.Skin);
   ```
   This retrieves the cosmetic from `SteamItemIdReference.Inst[itemId]` and renders the sprite on their screen via `_cat.SetItem()`.
4. **Validation Immunity**:
   `CatCosmetics.Validate()` filters `CatInventory.Instance.Items.Where(i => ids.Contains(i.Id) && i.RuntimeData.GetTotalQuantity() > 0)`.
   Because `GetTotalQuantity()` returns `999`, equipped items are never stripped or cleared on scene change.

### 8.2 Infinite Chest Claiming Architecture (`Shop` & `ChestExchanger`)
1. **Token Gate Resolution**:
   `ChestExchanger.ExchangeWhenReady` contains:
   ```csharp
   yield return new WaitUntil(() => !IsExchanging && 
       Game.Inventory.Achievement_Token.GetTotalQuantity() > 0 && 
       Game.Inventory.ManualToken.GetTotalQuantity() > 0);
   ```
   Our `GetTotalQuantity()` return value of `999` satisfies both `Achievement_Token` (97) and `ManualToken` (96), unblocking exchange execution immediately.
2. **Shop Refresh Timer Neutralization**:
   In vanilla, `Shop.ItemGotBought()` sets `StockRefreshTimeLeft = 1800` (30 mins) and writes to `TIME_LEFT_h1828194100` and `EMOTE_CHEST_TIME_LEFT_h1089283883`.
   `OnTick()` in `BongoTrainer.exe` continuously sweeps and zeroes all prefixed registry keys (`TIME_LEFT*`, `EMOTE_CHEST_TIME_LEFT*`, `TIME_LEFT_e*`) every second.
3. **Purchasing Economy Maintenance**:
   `ShopItem.CanBuy()` queries `Pets.Instance.CanSpendPets(_price)`.
   `OnTick()` continuously guarantees `BongoTap = 999999999` and `BongoMinus = 0`, ensuring endless continuous purchases without ever running out of currency.


