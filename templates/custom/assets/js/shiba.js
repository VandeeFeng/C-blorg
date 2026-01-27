const SHIBA_ANIMATIONS = {
    idle: 'shiba_idle_8fps.gif',
    walk: 'shiba_walk_8fps.gif',
    walkFast: 'shiba_walk_fast_8fps.gif',
    run: 'shiba_run_8fps.gif',
    lie: 'shiba_lie_8fps.gif',
    swipe: 'shiba_swipe_8fps.gif',
    withBall: 'shiba_with_ball_8fps.gif'
};

const ANIMATION_DURATIONS = {
    idle: 5000,
    walk: 3000,
    walkFast: 2500,
    run: 3000,
    lie: 8000,
    swipe: 8000,
    withBall: 3000
};

const ANIMATION_SPEEDS = {
    idle: 0,
    lie: 0,
    swipe: 0,
    walk: 1,
    walkFast: 2,
    run: 4
};

class ShibaAnimator {
    constructor() {
        this.shibaElement = document.querySelector('.shiba');
        this.currentAnimation = 'idle';
        this.isAnimating = false;
        this.position = 0;
        this.direction = 1; // 1 for right, -1 for left
        this.containerWidth = document.querySelector('header').offsetWidth;
        this.lastAnimationChange = 0;
        this.isPlayingWithBall = false;
        this.ballPlayTimer = null;
        
        // Initialize position
        this.initializePosition();
        
        // Make shiba draggable
        this.shibaElement.style.cursor = 'grab';
        this.shibaElement.addEventListener('mousedown', this.startDrag.bind(this));
        window.addEventListener('mousemove', this.drag.bind(this));
        window.addEventListener('mouseup', this.stopDrag.bind(this));
        
        // Add click handler for ball play
        this.shibaElement.addEventListener('click', this.toggleBallPlay.bind(this));
        
        // Handle window resize
        window.addEventListener('resize', () => {
            this.containerWidth = document.querySelector('header').offsetWidth;
            this.initializePosition();
        });
    }

    initializePosition() {
        this.shibaElement.style.transform = `scaleX(${this.direction})`;
    }

    toggleBallPlay(e) {
        // If we're dragging, ignore the click
        if (this.isDragging) return;
        
        // Clear any existing timers
        if (this.ballPlayTimer) {
            clearTimeout(this.ballPlayTimer);
            this.ballPlayTimer = null;
        }

        // Reset ball play state
        this.isPlayingWithBall = true;
        this.stop();
        
        // Start ball play sequence
        const startBallPlay = () => {
            this.setAnimation('withBall');
            this.ballPlayTimer = setTimeout(() => {
                this.isPlayingWithBall = false;
                this.ballPlayTimer = null;
                this.start();
            }, 3000);
        };

        setTimeout(startBallPlay, 200);
    }

    startDrag(e) {
        // Don't allow dragging while playing with ball
        if (this.isPlayingWithBall) return;
        
        this.isDragging = true;
        this.dragStartX = e.clientX - this.position;
        this.stop();
        this.shibaElement.style.cursor = 'grabbing';
    }

    drag(e) {
        if (!this.isDragging) return;
        this.position = e.clientX - this.dragStartX;
        this.updatePosition();
    }

    stopDrag() {
        this.isDragging = false;
        this.shibaElement.style.cursor = 'grab';
        this.start();
    }

    setAnimation(animationName) {
        // Don't change animation if playing with ball (unless explicitly setting to withBall)
        if (this.isPlayingWithBall && animationName !== 'withBall') return;
        
        this.shibaElement.src = `assets/shiba_gif/${SHIBA_ANIMATIONS[animationName]}`;
        this.currentAnimation = animationName;
        if (animationName !== 'idle') {
            this.lastAnimationChange = Date.now();
        }
    }

    updatePosition() {
        // Keep shiba within bounds
        if (this.position < 0) {
            this.position = 0;
            this.changeDirection(1);
        } else if (this.position > this.containerWidth - 100) {
            this.position = this.containerWidth - 100;
            this.changeDirection(-1);
        }
        
        this.shibaElement.style.left = `${this.position}px`;
    }

    changeDirection(newDirection) {
        if (this.direction === newDirection || this.isPlayingWithBall) return;
        
        this.direction = newDirection;
        const currentAnimation = this.currentAnimation;
        const lastChangeTime = this.lastAnimationChange; // Store the last change time
        const timeElapsedInAnimation = Date.now() - lastChangeTime; // Calculate time spent in current animation
        
        // Completely stop movement and animation
        this.isAnimating = false;
        this.stop();
        
        setTimeout(() => {
            this.shibaElement.style.transform = `scaleX(${this.direction})`;
            setTimeout(() => {
                // Only restart if we're not in a stopped state
                if (!this.isDragging && !this.isPlayingWithBall) {
                    this.isAnimating = true;
                    this.setAnimation(currentAnimation);
                    // Set lastAnimationChange to account for the time already spent in animation
                    this.lastAnimationChange = Date.now() - timeElapsedInAnimation;
                    this.move();
                }
            }, 300);
        }, 200);
    }

    async move() {
        if (!this.isAnimating || this.isDragging || this.isPlayingWithBall) return;

        // Only update position for movement animations AND when actively animating
        const isMovementAnimation = ['walk', 'walkFast', 'run'].includes(this.currentAnimation);
        if (isMovementAnimation && this.isAnimating) {
            const speed = ANIMATION_SPEEDS[this.currentAnimation] * this.direction;
            this.position += speed;
            this.updatePosition();
        }

        const now = Date.now();
        const timeSinceLastChange = now - this.lastAnimationChange;

        // Only allow animation changes if minimum duration has passed
        if (timeSinceLastChange > ANIMATION_DURATIONS[this.currentAnimation] || this.currentAnimation === 'idle') {
            // Randomly change animation
            if (Math.random() < 0.01) {
                // Add special animations with lower probability
                if (Math.random() < 0.2) {  // 20% chance for special animations
                    const specialAnimations = ['lie', 'swipe'];
                    const nextAnimation = specialAnimations[Math.floor(Math.random() * specialAnimations.length)];
                    this.setAnimation(nextAnimation);
                    this.lastAnimationChange = now;
                    
                    // Resume normal movement after duration
                    setTimeout(() => {
                        if (this.isAnimating && !this.isDragging && !this.isPlayingWithBall) {
                            const movementAnimations = ['walk', 'walkFast', 'run'];
                            this.setAnimation(movementAnimations[Math.floor(Math.random() * movementAnimations.length)]);
                            this.lastAnimationChange = now;
                        }
                    }, ANIMATION_DURATIONS[nextAnimation]);
                } else {
                    // 80% chance for normal movement animations
                    const movementAnimations = ['walk', 'walkFast', 'run'];
                    const nextAnimation = movementAnimations[Math.floor(Math.random() * movementAnimations.length)];
                    this.setAnimation(nextAnimation);
                    this.lastAnimationChange = now;
                }
            }

            // Randomly stop
            if (Math.random() < 0.005) {
                this.stop();
                setTimeout(() => this.start(), 2000 + Math.random() * 3000);
                return;
            }
        }

        requestAnimationFrame(() => this.move());
    }

    start() {
        if (this.isAnimating) return;
        this.isAnimating = true;
        const animations = ['walk', 'walkFast', 'run'];
        this.setAnimation(animations[Math.floor(Math.random() * animations.length)]);
        this.move();
    }

    stop() {
        this.isAnimating = false;
        // Don't change to idle if playing with ball
        if (!this.isPlayingWithBall) {
            this.setAnimation('idle');
        }
    }
}

// Start animation when page loads
document.addEventListener('DOMContentLoaded', () => {
    const shibaAnimator = new ShibaAnimator();
    shibaAnimator.start();
}); 