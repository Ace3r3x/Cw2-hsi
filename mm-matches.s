/*
 * ARM Assembly implementation of the matching algorithm for MasterMind
 * For F28HS Coursework 2
 *
 * Calculates exact matches (same color, same position) and
 * approximate matches (same color, different position)
 */

.global matchesASM

/*
 * Function: matchesASM
 * Parameters:
 *   r0 - pointer to secret array
 *   r1 - pointer to guess array
 *   r2 - length of arrays
 *   r3 - pointer to store exact matches
 *   [sp] - pointer to store approximate matches
 */
matchesASM:
    push {r4-r11, lr}       @ Save registers

    @ Initialize counters
    mov r4, #0              @ r4 = exact matches
    mov r5, #0              @ r5 = approximate matches
    
    @ Create temporary arrays on stack for marking used elements
    sub sp, sp, r2, lsl #2  @ Allocate space for secret_used array (length * 4 bytes)
    mov r6, sp              @ r6 = secret_used array
    sub sp, sp, r2, lsl #2  @ Allocate space for guess_used array (length * 4 bytes)
    mov r7, sp              @ r7 = guess_used array
    
    @ Initialize used arrays to 0
    mov r8, #0              @ Loop counter
init_loop:
    cmp r8, r2
    beq init_done
    str #0, [r6, r8, lsl #2]
    str #0, [r7, r8, lsl #2]
    add r8, r8, #1
    b init_loop
init_done:

    @ First pass: Find exact matches
    mov r8, #0              @ Loop counter
exact_loop:
    cmp r8, r2
    beq exact_done
    
    @ Load secret[i] and guess[i]
    ldr r9, [r0, r8, lsl #2]
    ldr r10, [r1, r8, lsl #2]
    
    @ Compare for exact match
    cmp r9, r10
    bne not_exact
    
    @ Mark as used
    mov r11, #1
    str r11, [r6, r8, lsl #2]
    str r11, [r7, r8, lsl #2]
    
    @ Increment exact matches
    add r4, r4, #1
    
not_exact:
    add r8, r8, #1
    b exact_loop
exact_done:

    @ Second pass: Find approximate matches
    mov r8, #0              @ Loop counter for secret
approx_outer_loop:
    cmp r8, r2
    beq approx_done
    
    @ Check if secret[i] is already used
    ldr r9, [r6, r8, lsl #2]
    cmp r9, #1
    beq next_secret
    
    @ Load secret[i]
    ldr r9, [r0, r8, lsl #2]
    
    @ Inner loop: check against all guess elements
    mov r10, #0             @ Loop counter for guess
approx_inner_loop:
    cmp r10, r2
    beq next_secret
    
    @ Check if guess[j] is already used
    ldr r11, [r7, r10, lsl #2]
    cmp r11, #1
    beq next_guess
    
    @ Load guess[j]
    ldr r11, [r1, r10, lsl #2]
    
    @ Compare for approximate match
    cmp r9, r11
    bne next_guess
    
    @ Mark as used
    mov r11, #1
    str r11, [r6, r8, lsl #2]
    str r11, [r7, r10, lsl #2]
    
    @ Increment approximate matches
    add r5, r5, #1
    
    @ Break inner loop after finding a match
    b next_secret
    
next_guess:
    add r10, r10, #1
    b approx_inner_loop
    
next_secret:
    add r8, r8, #1
    b approx_outer_loop
approx_done:

    @ Store exact matches result
    str r4, [r3]
    
    @ Load pointer to approximate matches from stack
    ldr r3, [sp, #(r2, lsl #3)]  @ Offset by 2*length*4 + original sp
    
    @ Store approximate matches result
    str r5, [r3]
    
    @ Clean up stack
    add sp, sp, r2, lsl #3  @ Deallocate both arrays (2*length*4 bytes)
    
    pop {r4-r11, pc}        @ Restore registers and return