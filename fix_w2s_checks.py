with open("star/features/esp.h", "r") as f:
    text = f.read()

# Replace any occurrence of `newHeadPos.x == -1` with `newHeadPos.x == -1 && newHeadPos.y == -1` ?
# The code checks `if (torso2D.x == -1 || torso2D.y == -1)` which is correct.
# Wait, let's see how `WorldToScreen` handles errors.
with open("star/rbx/globals/globals.cpp", "r") as f:
    print(f.read()[:500])
